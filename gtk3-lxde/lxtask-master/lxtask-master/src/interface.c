/* $Id: interface.c 3940 2008-02-10 22:48:45Z nebulon $
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 * Copyright (C) 2023-2024 Ingo Brückl
 * Updated by iloveengines1234 to support modern GTK 3.24.52 compilation environments
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

/* 2008-04-26 modified by Hong Jen Yee to be used in LXDE. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include "interface.h"
#include "utils.h"

/* --- Core Global UI Elements --- */
GtkTreeStore *list_store = NULL;
GtkTreeSelection *selection = NULL;
GtkWidget *treeview = NULL;
GtkWidget *mainmenu = NULL;
GtkWidget *taskpopup = NULL;
GtkWidget *cpu_usage_progress_bar = NULL;
GtkWidget *mem_usage_progress_bar = NULL;
GtkWidget *cpu_usage_progress_bar_box = NULL;
GtkWidget *mem_usage_progress_bar_box = NULL;

GtkTreeViewColumn *column[N_COLS];
gint column_width[N_COLS];

/* App Preferences Components */
void show_preferences(void);
extern gint refresh_interval;
extern guint rID;
static GtkWidget *refresh_spin = NULL;

const char *details(void)
{
    return full_view ? _("Less details") : _("More details");
}

GtkWidget* create_main_window(void)
{
    GtkWidget *window;
    GtkWidget *menubar, *menu, *item;
    GtkWidget *vbox1;
    GtkWidget *grid;
    GtkWidget *scrolledwindow1;
    GtkWidget *button1;
    GtkWidget *button3;
    GtkWidget *system_info_box;

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), _("Task Manager"));
    gtk_window_set_default_size(GTK_WINDOW(window), win_width, win_height);
    gtk_window_set_icon_name(GTK_WINDOW(window), "utilities-system-monitor");

    vbox1 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_show(vbox1);
    gtk_container_add(GTK_CONTAINER(window), vbox1);

    menubar = gtk_menu_bar_new();
    gtk_widget_show(menubar);
    gtk_box_pack_start(GTK_BOX(vbox1), menubar, FALSE, TRUE, 0);

    /* --- File Menu --- */
    menu = gtk_menu_new();
    item = gtk_menu_item_new_with_mnemonic(_("_File"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);

    item = gtk_menu_item_new_with_mnemonic(_("_Quit"));
    GtkAccelGroup *accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_Escape, 0, GTK_ACCEL_VISIBLE);
    gtk_widget_add_accelerator(item, "activate", accel_group, GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(gtk_main_quit), NULL);

    /* --- View Menu --- */
    item = gtk_menu_item_new_with_mnemonic(_("_View"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    menu = create_mainmenu();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    /* --- Help Menu --- */
    item = gtk_menu_item_new_with_mnemonic(_("_Help"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menubar), item);
    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

    item = gtk_menu_item_new_with_mnemonic(_("_About"));
    gtk_widget_show(item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(show_about_dialog), NULL);

    gtk_widget_show_all(menubar);

    /* --- Content Layout --- */
    GtkWidget *content_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(content_vbox), 6);
    gtk_widget_show(content_vbox);
    gtk_box_pack_start(GTK_BOX(vbox1), content_vbox, TRUE, TRUE, 0);

    system_info_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_set_homogeneous(GTK_BOX(system_info_box), TRUE);
    gtk_widget_show(system_info_box);
    gtk_box_pack_start(GTK_BOX(content_vbox), system_info_box, FALSE, TRUE, 0);

    /* Progress bars */
    cpu_usage_progress_bar_box = gtk_event_box_new();
    cpu_usage_progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(cpu_usage_progress_bar), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(cpu_usage_progress_bar), _("cpu usage"));
    gtk_widget_show(cpu_usage_progress_bar);
    gtk_widget_show(cpu_usage_progress_bar_box);
    gtk_container_add(GTK_CONTAINER(cpu_usage_progress_bar_box), cpu_usage_progress_bar);
    gtk_box_pack_start(GTK_BOX(system_info_box), cpu_usage_progress_bar_box, TRUE, TRUE, 0);

    mem_usage_progress_bar_box = gtk_event_box_new();
    mem_usage_progress_bar = gtk_progress_bar_new();
    gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(mem_usage_progress_bar), TRUE);
    gtk_progress_bar_set_text(GTK_PROGRESS_BAR(mem_usage_progress_bar), _("memory usage"));
    gtk_widget_show(mem_usage_progress_bar);
    gtk_widget_show(mem_usage_progress_bar_box);
    gtk_container_add(GTK_CONTAINER(mem_usage_progress_bar_box), mem_usage_progress_bar);
    gtk_box_pack_start(GTK_BOX(system_info_box), mem_usage_progress_bar_box, TRUE, TRUE, 0);

    scrolledwindow1 = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_show(scrolledwindow1);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(content_vbox), scrolledwindow1, TRUE, TRUE, 0);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrolledwindow1), GTK_SHADOW_IN);

    treeview = gtk_tree_view_new();
    gtk_widget_show(treeview);
    gtk_container_add(GTK_CONTAINER(scrolledwindow1), treeview);

    create_list_store();

    selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
    gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(list_store));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(list_store), COLUMN_TIME, GTK_SORT_DESCENDING);

    create_taskpopup(accel_group);

    grid = gtk_grid_new();
    gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
    gtk_box_pack_start(GTK_BOX(content_vbox), grid, FALSE, TRUE, 0);
    gtk_widget_show(grid);

    button3 = gtk_button_new_with_label(details());
    gtk_widget_set_focus_on_click(button3, FALSE);
    gtk_widget_show(button3);
    gtk_grid_attach(GTK_GRID(grid), button3, 0, 0, 1, 1);

    button1 = gtk_button_new_with_mnemonic(_("_Quit"));
    gtk_widget_set_halign(button1, GTK_ALIGN_END);
    gtk_widget_set_valign(button1, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(button1, TRUE);
    gtk_grid_attach(GTK_GRID(grid), button1, 1, 0, 1, 1);
    gtk_widget_show(button1);

    g_signal_connect(window, "destroy", G_CALLBACK(on_quit), NULL);
    g_signal_connect_swapped(treeview, "button-press-event", G_CALLBACK(on_treeview1_button_press_event), NULL);
    g_signal_connect(treeview, "popup-menu", G_CALLBACK(on_treeview_popup_menu), NULL);
    g_signal_connect(button1, "clicked", G_CALLBACK(on_quit), NULL);
    g_signal_connect(button3, "clicked", G_CALLBACK(on_button3_toggled_event), NULL);

    return window;
}

void create_list_store(void)
{
    GtkCellRenderer *cell_renderer;
    GType types[N_COLS] = { G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING };

    list_store = gtk_tree_store_newv(N_COLS, types);
    cell_renderer = gtk_cell_renderer_text_new();

    const gchar *titles[N_COLS] = { _("Command"), _("User"), _("CPU"), _("RSS"), _("VM-Size"), _("PID"), _("State"), _("Prio"), _("PPID") };
    GtkTreeIterCompareFunc funcs[N_COLS] = { compare_string_list_item, compare_string_list_item, compare_double_list_item, compare_size_list_item, compare_size_list_item, compare_int_list_item, compare_string_list_item, compare_int_list_item, compare_int_list_item };

    for (gint col_idx = 0; col_idx < N_COLS; col_idx++) {
        column[col_idx] = gtk_tree_view_column_new_with_attributes(titles[col_idx], cell_renderer, "text", col_idx, NULL);
        if (column_width[col_idx] > 0) {
            gtk_tree_view_column_set_sizing(column[col_idx], GTK_TREE_VIEW_COLUMN_FIXED);
            gtk_tree_view_column_set_fixed_width(column[col_idx], column_width[col_idx]);
        }
        gtk_tree_view_column_set_resizable(column[col_idx], TRUE);
        gtk_tree_view_column_set_sort_column_id(column[col_idx], col_idx);
        gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(list_store), col_idx, funcs[col_idx], GUINT_TO_POINTER(col_idx), NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(treeview), column[col_idx]);
        
        if (col_idx >= 2 && col_idx <= 5) {
            g_object_set(cell_renderer, "xalign", 1.0, NULL);
        } else {
            g_object_set(cell_renderer, "xalign", 0.0, NULL);
        }
    }
    change_list_store_view();
}

void create_taskpopup(GtkAccelGroup *accel_group)
{
    GtkWidget *menu_item;
    taskpopup = gtk_menu_new();

    const struct { const gchar *label; const gchar *signal; gchar *accel; GdkModifierType mask; } items[] = {
        { _("Stop"), "STOP", NULL, 0 },
        { _("Continue"), "CONT", NULL, 0 },
        { _("Term"), "TERM", "Delete", 0 },
        { _("Kill"), "KILL", "Delete", GDK_SHIFT_MASK }
    };

    for (guint i = 0; i < G_N_ELEMENTS(items); i++) {
        menu_item = gtk_menu_item_new_with_mnemonic(items[i].label);
        if (items[i].accel) {
            guint key = strcmp(items[i].accel, "Delete") == 0 ? GDK_KEY_Delete : 0;
            gtk_widget_add_accelerator(menu_item, "activate", accel_group, key, items[i].mask, GTK_ACCEL_VISIBLE);
        }
        gtk_widget_show(menu_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(taskpopup), menu_item);
        g_signal_connect(menu_item, "activate", G_CALLBACK(handle_task_menu), (gpointer)items[i].signal);
    }

    menu_item = gtk_menu_item_new_with_mnemonic(_("Priority"));
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), create_prio_submenu());
    gtk_widget_show(menu_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(taskpopup), menu_item);
}

GtkWidget *create_prio_submenu(void)
{
    GtkWidget *prio_submenu = gtk_menu_new();
    const gchar *prios[] = { " -10", "  -5", "    0", "    5", "   10" };

    for (guint i = 0; i < G_N_ELEMENTS(prios); i++) {
        GtkWidget *menu_item = gtk_menu_item_new_with_mnemonic(prios[i]);
        gtk_widget_show(menu_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(prio_submenu), menu_item);
        g_signal_connect(menu_item, "activate", G_CALLBACK(handle_prio_menu), (gpointer)prios[i]);
    }
    return prio_submenu;
}

GtkWidget* create_mainmenu(void)
{
    GtkWidget *mainmenu_w = gtk_menu_new();
    GtkAccelGroup *accel_group = gtk_accel_group_new();

    struct { GtkWidget **w; const gchar *label; gboolean active; GCallback cb; gpointer data; } items[] = {
        { NULL, _("Show user tasks"), show_user_tasks, G_CALLBACK(on_show_tasks_toggled), (gpointer)(gsize)own_uid },
        { NULL, _("Show root tasks"), show_root_tasks, G_CALLBACK(on_show_tasks_toggled), (gpointer)0 },
        { NULL, _("Show other tasks"), show_other_tasks, G_CALLBACK(on_show_tasks_toggled), (gpointer)-1 },
        { NULL, _("Show full cmdline"), show_full_path, G_CALLBACK(on_show_tasks_toggled), (gpointer)-2 },
        { NULL, _("Show memory used by cache as free"), show_cached_as_free, G_CALLBACK(on_show_cached_as_free_toggled), (gpointer)0 }
    };

    for (guint i = 0; i < G_N_ELEMENTS(items); i++) {
        GtkWidget *menu_item = gtk_check_menu_item_new_with_mnemonic(items[i].label);
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item), items[i].active);
        gtk_widget_show(menu_item);
        gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu_w), menu_item);
        g_signal_connect(menu_item, "toggled", items[i].cb, items[i].data);
    }

    GtkWidget *sep = gtk_separator_menu_item_new();
    gtk_widget_show(sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu_w), sep);

    GtkWidget *prefs = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
    g_signal_connect(prefs, "activate", G_CALLBACK(show_preferences), NULL);
    gtk_widget_show(prefs);
    gtk_menu_shell_append(GTK_MENU_SHELL(mainmenu_w), prefs);

    gtk_menu_set_accel_group(GTK_MENU(mainmenu_w), accel_group);
    return mainmenu_w;
}

void show_about_dialog(void)
{
    const gchar *authors[] = {
        "Hong Jen Yee <pcman.tw@gmail.com>",
        "Jan Dlabal <dlabaljan@gmail.com>",
        "Johannes Zellner <webmaster@nebulon.de>",
        NULL
    };

    GtkWidget *about_dlg = gtk_about_dialog_new();
    gtk_container_set_border_width(GTK_CONTAINER(about_dlg), 2);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dlg), VERSION);
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dlg), _("LXTask"));
    gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about_dlg), "utilities-system-monitor");
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dlg), _("Copyright (C) 2008-2026 LXDE team"));
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dlg), _("Lightweight task manager for LXDE project"));
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dlg), "http://lxde.org/");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dlg), authors);

    gtk_dialog_run(GTK_DIALOG(about_dlg));
    gtk_widget_destroy(about_dlg);
}

void change_list_store_view(void)
{
    gboolean show_extended = full_view;
    gtk_tree_view_column_set_visible(column[COLUMN_PPID], show_extended);
    gtk_tree_view_column_set_visible(column[COLUMN_MEM], show_extended);
    gtk_tree_view_column_set_visible(column[COLUMN_STATE], show_extended);
    gtk_tree_view_column_set_visible(column[COLUMN_PRIO], show_extended);
    gtk_tree_view_column_set_visible(column[COLUMN_UNAME], (show_root_tasks || show_other_tasks) ? TRUE : show_extended);
}

void fill_list_item(guint i, GtkTreeIter *iter)
{
    if (!iter) return;

    char buf[64];
    struct task *task = &g_array_index(task_array, struct task, i);

    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_NAME, task->name, -1);
    
    snprintf(buf, sizeof(buf), "%u", (guint)task->pid);
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_PID, buf, -1);
    
    snprintf(buf, sizeof(buf), "%u", (guint)task->ppid);
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_PPID, buf, -1);
    
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_STATE, task->state, -1);

    /* FIXED: Correctly manage and free strings allocated by size_to_string to prevent leaks */
    gchar *mem_str = size_to_string((guint64)task->size * 1024);
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_MEM, mem_str, -1);
    g_free(mem_str);

    gchar *rss_str = size_to_string((guint64)task->rss * 1024);
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_RSS, rss_str, -1);
    g_free(rss_str);

    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_UNAME, task->uname, -1);
    
    snprintf(buf, sizeof(buf), _("%0.1f%%"), task->time_percentage);
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_TIME, buf, -1);
    
    snprintf(buf, sizeof(buf), "%d", task->prio);
    gtk_tree_store_set(GTK_TREE_STORE(list_store), iter, COLUMN_PRIO, buf, -1);
}

void add_new_list_item(guint i)
{
    GtkTreeIter iter;
    gtk_tree_store_append(GTK_TREE_STORE(list_store), &iter, NULL);
    fill_list_item(i, &iter);
}

void refresh_list_item(guint i)
{
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);
    struct task *task = &g_array_index(task_array, struct task, i);
    
    while (valid) {
        gchar *str_data = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, COLUMN_PID, &str_data, -1);

        if (str_data && task->pid == (pid_t)strtol(str_data, NULL, 10)) {
            g_free(str_data);
            fill_list_item(i, &iter);
            return;
        }
        g_free(str_data);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
    }
}

void remove_list_item(pid_t pid)
{
    GtkTreeIter iter;
    gboolean valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(list_store), &iter);

    while (valid) {
        gchar *str_data = NULL;
        gtk_tree_model_get(GTK_TREE_MODEL(list_store), &iter, COLUMN_PID, &str_data, -1);

        if (str_data && pid == (pid_t)strtol(str_data, NULL, 10)) {
            g_free(str_data);
            gtk_tree_store_remove(GTK_TREE_STORE(list_store), &iter);
            return;
        }
        g_free(str_data);
        valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(list_store), &iter);
    }
}

/* --- Type-Safe Fast Sorting Comparators --- */
gint compare_int_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column)
{
    gchar *s1, *s2;
    gtk_tree_model_get(model, iter1, GPOINTER_TO_UINT(column), &s1, -1);
    gtk_tree_model_get(model, iter2, GPOINTER_TO_UINT(column), &s2, -1);

    gint i1 = s1 ? atoi(s1) : 0;
    gint i2 = s2 ? atoi(s2) : 0;
    g_free(s1); g_free(s2);

    return i1 - i2;
}

gint compare_double_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column)
{
    gchar *s1, *s2;
    gtk_tree_model_get(model, iter1, GPOINTER_TO_UINT(column), &s1, -1);
    gtk_tree_model_get(model, iter2, GPOINTER_TO_UINT(column), &s2, -1);

    gdouble d1 = s1 ? g_ascii_strtod(s1, NULL) : 0.0;
    gdouble d2 = s2 ? g_ascii_strtod(s2, NULL) : 0.0;
    g_free(s1); g_free(s2);

    return (d1 > d2) - (d1 < d2);
}

gint compare_size_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column)
{
    gchar *s1, *s2;
    gtk_tree_model_get(model, iter1, GPOINTER_TO_UINT(column), &s1, -1);
    gtk_tree_model_get(model, iter2, GPOINTER_TO_UINT(column), &s2, -1);

    /* String token resolution remains efficient via direct numeric conversion */
    guint64 i1 = s1 ? string_to_size(s1) : 0;
    guint64 i2 = s2 ? string_to_size(s2) : 0;
    g_free(s1); g_free(s2);

    return (i1 > i2) - (i1 < i2);
}

gint compare_string_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column)
{
    gchar *s1, *s2;
    gtk_tree_model_get(model, iter1, GPOINTER_TO_UINT(column), &s1, -1);
    gtk_tree_model_get(model, iter2, GPOINTER_TO_UINT(column), &s2, -1);

    gint ret = (s1 && s2) ? g_ascii_strcasecmp(s1, s2) : 0;
    g_free(s1); g_free(s2);
    return ret;
}

void change_task_view(void)
{
    gtk_tree_store_clear(GTK_TREE_STORE(list_store));
    change_list_store_view();

    for (gint i = 0; i < tasks; i++) {
        struct task *task = &g_array_index(task_array, struct task, i);
        if ((task->uid == own_uid && show_user_tasks) || 
            (task->uid == 0 && show_root_tasks) || 
            (task->uid != own_uid && task->uid != 0 && show_other_tasks)) {
            add_new_list_item(i);
        }
    }
    refresh_task_list();
}

void change_full_path(void)
{
    GArray *new_task_list = (GArray*)get_task_list();
    if (!new_task_list) return;

    /* FIXED: Enforce a bound ceiling loop check to prevent overflows */
    guint max_len = MIN(task_array->len, new_task_list->len);
    for (guint i = 0; i < max_len; i++) {
        struct task *tmp = &g_array_index(task_array, struct task, i);
        struct task *new_tmp = &g_array_index(new_task_list, struct task, i);
        
        /* FIXED: Expanded boundary from 255 to 1024 to preserve full cmdline structures */
        g_strlcpy(tmp->name, new_tmp->name, sizeof(tmp->name));
        refresh_list_item(i);
    }
    g_array_free(new_task_list, TRUE);
}

void apply_prefs()
{
    if (rID > 0) g_source_remove(rID);
    refresh_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(refresh_spin));
    
    /* FIXED: Modernized alignment with low-power event tracking rules */
    rID = g_timeout_add_seconds(refresh_interval, G_SOURCE_FUNC(refresh_task_list), NULL);
}

void show_preferences(void)
{
    GtkWidget *dlg, *c_area;
    GtkWidget *notebook = gtk_notebook_new();
    GtkWidget *general_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    GtkWidget *refresh_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    
    refresh_spin = gtk_spin_button_new_with_range(1, 60, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(refresh_spin), refresh_interval);

    dlg = gtk_dialog_new_with_buttons(_("Preferences"), NULL, 0, _("_Cancel"), GTK_RESPONSE_REJECT, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
    c_area = gtk_dialog_get_content_area(GTK_DIALOG(dlg));

    gtk_box_pack_start(GTK_BOX(c_area), notebook, TRUE, TRUE, 0);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), general_box, gtk_label_new(_("General")));
    gtk_box_pack_start(GTK_BOX(refresh_box), gtk_label_new(_("Refresh rate (seconds):")), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(refresh_box), refresh_spin, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(general_box), refresh_box, TRUE, TRUE, 0);

    gtk_widget_show_all(notebook);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
        apply_prefs();
    }
    gtk_widget_destroy(dlg);
}