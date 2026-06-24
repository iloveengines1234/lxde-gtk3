/*
 * xfce4-taskmanager - very simple taskmanger
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 * Copyright (C) 2024 Ingo Brückl
 * Updated by iloveengines1234 to support modern GTK 3 compilation environments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library  General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <stdlib.h>
#include <unistd.h>

#include "callbacks.h"
#include "interface.h"

extern guint rID;

/* --- View Toggle Logic --- */
void on_view_filter_toggled(GtkToggleButton *button, gpointer user_data)
{
    /* Toggle application view state cleanly */
    full_view = !full_view;
    
    /* Safely update labels without relying on implicit type coercions */
    gtk_button_set_label(GTK_BUTTON(button), details());
    change_list_store_view();
}


/* --- Contextual UI Event Invocations --- */
gboolean on_treeview1_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    gchar *signal_str;
    
    /* Modern GTK3 event property fields abstraction isolation checks */
    if (event->type == GDK_BUTTON_PRESS && event->button == 3)
    {
        if (taskpopup) 
        {
            /* Native GTK3 pointer mapping replacement for deprecated gtk_menu_popup layers */
            gtk_menu_popup_at_pointer(GTK_MENU(taskpopup), (GdkEvent *)event);
            return TRUE;
        }
    }
    return FALSE;
}

gboolean on_treeview_popup_menu(GtkWidget *widget, gpointer user_data)
{
    GtkTreeView *treeview = GTK_TREE_VIEW(widget);
    GtkTreeSelection *local_selection = gtk_tree_view_get_selection(treeview);
    GtkTreeModel *model;
    GtkTreeIter iter;

    if (gtk_tree_selection_get_selected(local_selection, &model, &iter))
    {
        GtkTreePath *path;
        GdkRectangle rect;
        GdkWindow *bin_window;

        path = gtk_tree_model_get_path(model, &iter);
        gtk_tree_view_scroll_to_cell(treeview, path, NULL, FALSE, 0.0, 0.0);
        gtk_tree_view_get_cell_area(treeview, path, gtk_tree_view_get_column(treeview, COLUMN_NAME), &rect);
        gtk_tree_path_free(path);

        bin_window = gtk_tree_view_get_bin_window(treeview);
        if (bin_window && taskpopup) 
        {
            gtk_menu_popup_at_rect(GTK_MENU(taskpopup),
                                   bin_window,
                                   &rect,
                                   GDK_GRAVITY_SOUTH_WEST,
                                   GDK_GRAVITY_NORTH_WEST,
                                   NULL);
            return TRUE;
        }
    }
    return FALSE;
}


/* --- System Process Control Handlers --- */
void handle_task_menu(GtkWidget *widget, const gchar *signal_type)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *local_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview1));

    if (signal_type && gtk_tree_selection_get_selected(local_selection, &model, &iter))
    {
        gint task_action = SIGNAL_NO;

        switch(signal_type[0])
        {
            case 'K':
                if (confirm(_("Really kill the task?"), GTK_RESPONSE_NO))
                    task_action = SIGNAL_KILL;
                break;
            case 'T':
                if (confirm(_("Really terminate the task?"), GTK_RESPONSE_YES))
                    task_action = SIGNAL_TERM;
                break;
            case 'S':
                task_action = SIGNAL_STOP;
                break;
            case 'C':
                task_action = SIGNAL_CONT;
                break;
            default:
                return;
        }

        if (task_action != SIGNAL_NO)
            {
            gchar *task_id = NULL;

            gtk_tree_model_get(model, &iter, COLUMN_PID, &task_id, -1);
            if (task_id) 
            {
                pid_t target_pid = (pid_t)strtol(task_id, NULL, 10);
                
                if (target_pid == getpid() && task_action == SIGNAL_STOP)
                {
                    show_error(_("Can't stop process self"));
                }
                else
                {
                    send_signal_to_task(target_pid, task_action);
                    refresh_task_list();
                }
                g_free(task_id);
            }
        }
    }
}

void handle_prio_menu(GtkWidget *widget, const gchar *priority_level)
{
    gchar *task_id = NULL;
    GtkTreeModel *model;
    GtkTreeIter iter;
    GtkTreeSelection *local_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview1));

    if (priority_level && gtk_tree_selection_get_selected(local_selection, &model, &iter))
    {
        gtk_tree_model_get(model, &iter, COLUMN_PID, &task_id, -1);
        if (task_id)
        {
            pid_t target_pid = (pid_t)strtol(task_id, NULL, 10);
            gint prio_val = (gint)strtol(priority_level, NULL, 10);
            
            set_priority_to_task(target_pid, prio_val);
            refresh_task_list();
            g_free(task_id);
        }
    }
}


/* --- Filter Subsystem Routing Hooks --- */
void on_show_tasks_toggled(GtkCheckMenuItem *menu_item, gpointer user_data)
{
    /* Use size_t translation macros to guarantee safe 64-bit register sizing */
    gsize scope_id = GPOINTER_TO_SIZE(user_data);
    gboolean is_active = gtk_check_menu_item_get_active(menu_item);

    if (scope_id == (gsize)own_uid)
        show_user_tasks = is_active;
    else if (scope_id == 0)
        show_root_tasks = is_active;
    else if (scope_id == (gsize)-1)
        show_other_tasks = is_active;
    else {
        show_full_path = is_active;
        change_full_path();
        return;
    }

    change_task_view();
}

void on_show_cached_as_free_toggled(GtkCheckMenuItem *menu_item, gpointer user_data)
{
    show_cached_as_free = gtk_check_menu_item_get_active(menu_item);
    change_task_view();
}


/* --- Application Lifetime Destruction Block --- */
void on_quit(void)
{
    if (rID > 0) {
        g_source_remove(rID);
        rID = 0;
    }
    
    save_config();
    
    if (config_file) {
        g_free(config_file);
        config_file = NULL;
    }

    gtk_main_quit();
}