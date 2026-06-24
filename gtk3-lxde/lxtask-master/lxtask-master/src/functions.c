/* $Id: functions.c 3940 2008-02-10 22:48:45Z nebulon $
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 * Copyright (C) 2024 Ingo Brückl
 * Updated to support modern GTK 3 compilation environments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "functions.h"
#include "utils.h"

extern gint refresh_interval;

static system_status *sys_stat = NULL;

/* --- Local Structural Definitions --- */
typedef struct {
    GtkTreeIter iter;
    gboolean valid;
} TreeIterCache;

gboolean refresh_task_list(void)
{
    guint i;
    GArray *new_task_list;
    gdouble cpu_usage;
    guint num_cpus;
    guint64 memory_used;
    char tooltip[256];
    const gchar *current_pb_text;

    num_cpus = (sys_stat != NULL) ? sys_stat->cpu_count : 1;
    if (num_cpus == 0) num_cpus = 1;

    new_task_list = (GArray*)get_task_list();
    if (!new_task_list) return TRUE;

    /* --- OPTIMIZATION STEP: O(N) Hash Table Indexing --- */
    GHashTable *new_tasks_hash = g_hash_table_new(g_direct_hash, g_direct_equal);
    for (i = 0; i < new_task_list->len; i++) {
        struct task *nt = &g_array_index(new_task_list, struct task, i);
        nt->checked = FALSE;
        g_hash_table_insert(new_tasks_hash, GINT_TO_POINTER((gint)nt->pid), nt);
    }

    /* --- OPTIMIZATION STEP: O(N) GtkTreeStore View Cache --- */
    GHashTable *ui_cache = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_free);
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
    while (valid) {
        gchar *str_pid = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, COLUMN_PID, &str_pid, -1);
        if (str_pid) {
            pid_t pid = (pid_t)strtol(str_pid, NULL, 10);
            g_free(str_pid);
            
            TreeIterCache *cache_entry = g_new(TreeIterCache, 1);
            cache_entry->iter = iter;
            cache_entry->valid = TRUE;
            g_hash_table_insert(ui_cache, GINT_TO_POINTER((gint)pid), cache_entry);
        }
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
    }

    /* Track changes using the high-performance hash tables */
    for (i = 0; i < task_array->len; i++) {
        struct task *tmp = &g_array_index(task_array, struct task, i);
        tmp->checked = FALSE;

        struct task *new_tmp = g_hash_table_lookup(new_tasks_hash, GINT_TO_POINTER((gint)tmp->pid));
        if (new_tmp) {
            tmp->old_time = tmp->time;
            tmp->time = new_tmp->time;

            if (tmp->time >= tmp->old_time && refresh_interval > 0) {
                /* System handles dynamic ticks-per-second scaling variations natively */
                new_tmp->time_percentage = ((gfloat)(tmp->time - tmp->old_time) / (gfloat)(refresh_interval * num_cpus));
                if (new_tmp->time_percentage > 100.0f) new_tmp->time_percentage = 100.0f;
            } else {
                new_tmp->time_percentage = 0.0f;
            }

            gboolean changed = (tmp->ppid != new_tmp->ppid ||
                                 tmp->state[0] != new_tmp->state[0] ||
                                 tmp->size != new_tmp->size ||
                                 tmp->rss != new_tmp->rss ||
                                 ABS(tmp->time_percentage - new_tmp->time_percentage) > 0.01f ||
                                 tmp->prio != new_tmp->prio);

            tmp->ppid = new_tmp->ppid;
            tmp->state[0] = new_tmp->state[0];
            tmp->size = new_tmp->size;
            tmp->rss = new_tmp->rss;
            tmp->prio = new_tmp->prio;
            tmp->time_percentage = new_tmp->time_percentage;
            tmp->checked = TRUE;
            new_tmp->checked = TRUE;

            if (changed) {
                TreeIterCache *ui_entry = g_hash_table_lookup(ui_cache, GINT_TO_POINTER((gint)tmp->pid));
                if (ui_entry && ui_entry->valid) {
                    fill_list_item(i, &(ui_entry->iter));
                }
            }
        }
    }

    /* Remove obsolete tasks */
    i = 0;
    while (i < task_array->len) {
        struct task *tmp = &g_array_index(task_array, struct task, i);
        if (!tmp->checked) {
            remove_list_item(tmp->pid);
            g_array_remove_index(task_array, i);
            tasks--;
        } else {
            i++;
        }
    }

    /* Add brand new tasks */
    for (i = 0; i < new_task_list->len; i++) {
        struct task *new_tmp = &g_array_index(new_task_list, struct task, i);
        if (!new_tmp->checked) {
            g_array_append_val(task_array, *new_tmp);
            if ((show_user_tasks && new_tmp->uid == own_uid) || 
                (show_root_tasks && new_tmp->uid == 0) || 
                (show_other_tasks && new_tmp->uid != own_uid && new_tmp->uid != 0)) {
                add_new_list_item(tasks);
            }
            tasks++;
        }
    }

    /* Cleanup tracking collections */
    g_hash_table_destroy(new_tasks_hash);
    g_hash_table_destroy(ui_cache);
    g_array_free(new_task_list, TRUE);

    /* --- Refresh Global Progress Widgets --- */
    if (sys_stat == NULL) {
        sys_stat = g_new0(system_status, 1);
    }
    get_system_status(sys_stat);

    memory_used = sys_stat->mem_total - sys_stat->mem_free;
    if (show_cached_as_free) {
        guint64 cache_mem = sys_stat->mem_cached + sys_stat->mem_buffered;
        memory_used = (memory_used >= cache_mem) ? (memory_used - cache_mem) : 0;
    }

    snprintf(tooltip, sizeof(tooltip), _("Memory: %d MB of %d MB used"), 
             (gint)(memory_used / 1024), (gint)(sys_stat->mem_total / 1024));

    current_pb_text = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(mem_usage_progress_bar));
    if (current_pb_text == NULL || strcmp(tooltip, current_pb_text) != 0) {
        gdouble fraction = (sys_stat->mem_total > 0) ? (gdouble)memory_used / sys_stat->mem_total : 0.0;
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(mem_usage_progress_bar), fraction);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mem_usage_progress_bar), tooltip);
    }

    cpu_usage = get_cpu_usage(sys_stat);
    snprintf(tooltip, sizeof(tooltip), (cpu_usage == 0.0 || cpu_usage >= 1.0) ? _("CPU usage: %0.0f%%") : _("CPU usage: %0.1f%%"), cpu_usage * 100.0);

    current_pb_text = gtk_progress_bar_get_text(GTK_PROGRESS_BAR(cpu_usage_progress_bar));
    if (current_pb_text == NULL || strcmp(tooltip, current_pb_text) != 0) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cpu_usage_progress_bar), cpu_usage);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(cpu_usage_progress_bar), tooltip);
    }

    return TRUE;
}

gdouble get_cpu_usage(system_status *status_block)
{
    gdouble cpu_usage = 0.0;
    guint current_jiffies, current_used, delta_jiffies;

    if (get_cpu_usage_from_proc(status_block) == FALSE) {
        for (guint i = 0; i < task_array->len; i++) {
            struct task *tmp = &g_array_index(task_array, struct task, i);
            cpu_usage += tmp->time_percentage;
        }
        cpu_usage = cpu_usage / (status_block->cpu_count * 100.0);
    } else {
        if (status_block->cpu_old_jiffies > 0) {
            current_used = status_block->cpu_user + status_block->cpu_nice + status_block->cpu_system;
            current_jiffies = current_used + status_block->cpu_idle;

            if (current_jiffies >= status_block->cpu_old_jiffies) {
                delta_jiffies = current_jiffies - status_block->cpu_old_jiffies;
                if (delta_jiffies > 0 && current_used >= status_block->cpu_old_used) {
                    cpu_usage = (gdouble)(current_used - status_block->cpu_old_used) / (gdouble)delta_jiffies;
                }
            }
        }
    }
    return cpu_usage;
}

static gboolean key_file_get_bool(GKeyFile* kf, const char* group, const char* name, gboolean def)
{
    GError* err = NULL;
    gboolean ret = g_key_file_get_boolean(kf, group, name, &err);
    if (err) {
        ret = def;
        g_error_free(err);
    }
    return ret;
}

static gint key_file_get_int(GKeyFile* kf, const char* group, const char* name, gint def)
{
    GError* err = NULL;
    gint ret = g_key_file_get_integer(kf, group, name, &err);
    if (err) {
        ret = def;
        g_error_free(err);
    }
    return ret;
}

static void key_file_get_int_list(GKeyFile *kf, const char *group, const char *name, gint *ret, gsize len)
{
    gsize length = 0;
    GError *err = NULL;
    gint *list = g_key_file_get_integer_list(kf, group, name, &length, &err);

    if (err) {
        g_error_free(err);
        return;
    }

    for (gsize i = 0; i < len && i < length; i++) {
        ret[i] = list[i];
    }
    g_free(list);
}

void load_config(void)
{
    static const char group[] = "General";
    GKeyFile *rc_file = g_key_file_new();
    
    if (g_key_file_load_from_file(rc_file, config_file, G_KEY_FILE_NONE, NULL)) {
        show_user_tasks = key_file_get_bool(rc_file, group, "show_user_tasks", TRUE);
        show_root_tasks = key_file_get_bool(rc_file, group, "show_root_tasks", FALSE);
        show_other_tasks = key_file_get_bool(rc_file, group, "show_other_tasks", FALSE);
        show_full_path = key_file_get_bool(rc_file, group, "show_full_path", FALSE);
        show_cached_as_free = key_file_get_bool(rc_file, group, "show_cached_as_free", TRUE);
        full_view = key_file_get_bool(rc_file, group, "full_view", TRUE);
        win_width = key_file_get_int(rc_file, group, "win_width", 600);
        win_height = key_file_get_int(rc_file, group, "win_height", 400);
        refresh_interval = key_file_get_int(rc_file, group, "refresh_interval", 2);

        key_file_get_int_list(rc_file, group, "column_widths", column_width, N_COLS);
    }
    g_key_file_free(rc_file);
}

void save_config(void)
{
    GKeyFile *rc_file;
    gchar *data;
    gsize length = 0;
    GError *error = NULL;

    rc_file = g_key_file_new();
    
    g_key_file_set_boolean(rc_file, "General", "show_user_tasks", show_user_tasks);
    g_key_file_set_boolean(rc_file, "General", "show_root_tasks", show_root_tasks);
    g_key_file_set_boolean(rc_file, "General", "show_other_tasks", show_other_tasks);
    g_key_file_set_boolean(rc_file, "General", "show_full_path", show_full_path);
    g_key_file_set_boolean(rc_file, "General", "show_cached_as_free", show_cached_as_free);
    g_key_file_set_boolean(rc_file, "General", "full_view", full_view);

    if (main_window) {
        gtk_window_get_size(GTK_WINDOW(main_window), &win_width, &win_height);
    }

    g_key_file_set_integer(rc_file, "General", "win_width", win_width);
    g_key_file_set_integer(rc_file, "General", "win_height", win_height);
    g_key_file_set_integer(rc_file, "General", "refresh_interval", refresh_interval);

    gint current_widths[N_COLS];
    for (guint i = 0; i < N_COLS; i++) {
        if (column[i]) {
            current_widths[i] = gtk_tree_view_column_get_width(column[i]);
        } else {
            current_widths[i] = column_width[i];
        }
    }
    g_key_file_set_integer_list(rc_file, "General", "column_widths", current_widths, N_COLS);

    data = g_key_file_to_data(rc_file, &length, NULL);
    if (data) {
        g_file_set_contents(config_file, data, length, &error);
        if (error) {
            g_error_free(error);
        }
        g_free(data);
    }
    g_key_file_free(rc_file);
}