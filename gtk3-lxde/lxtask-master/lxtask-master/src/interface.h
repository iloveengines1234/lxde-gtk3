/* $Id: interface.h 3940 2008-02-10 22:48:45Z nebulon $
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 * Copyright (C) 2024 Ingo Brückl
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

#ifndef INTERFACE_H
#define INTERFACE_H

#include <gtk/gtk.h>
#include <sys/types.h> /* Included explicitly for the pid_t type definition */

G_BEGIN_DECLS

/* --- Data Store Model Enumerations --- */
enum {
    COLUMN_NAME = 0,
    COLUMN_UNAME,
    COLUMN_TIME,
    COLUMN_RSS,
    COLUMN_MEM,
    COLUMN_PID,
    COLUMN_STATE,
    COLUMN_PRIO,
    COLUMN_PPID,
    N_COLS
};

/* --- Global Interface Component Handlers --- */
extern GtkTreeStore *list_store;
extern GtkTreeSelection *selection;
extern GtkWidget *treeview;
extern GtkWidget *mainmenu;
extern GtkWidget *taskpopup;
extern GtkWidget *cpu_usage_progress_bar;
extern GtkWidget *mem_usage_progress_bar;
extern GtkWidget *cpu_usage_progress_bar_box;
extern GtkWidget *mem_usage_progress_bar_box;

/* --- Layout Construction & Window Initialization --- */
GtkWidget *create_main_window(void);
GtkWidget *create_mainmenu(void);
GtkWidget *create_prio_submenu(void);
void create_taskpopup(GtkAccelGroup *accel_group);
void show_about_dialog(void);

/* --- Tree View Modification & Data Management --- */
void create_list_store(void);
void change_list_store_view(void);
void change_task_view(void);
void change_full_path(void);

void fill_list_item(guint i, GtkTreeIter *iter);
void add_new_list_item(guint i);
void refresh_list_item(guint i);
void remove_list_item(pid_t pid);

/* --- GTK 3 Optimized Model Comparison Sort Routines --- */
gint compare_list_item(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
gint compare_int_list_item(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
gint compare_double_list_item(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
gint compare_size_list_item(GtkTreeModel *model, GtkTreeIter *iter1, GtkTreeIter *iter2, gpointer column);
gint compare_string_list_item(GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);

const char *details(void);

G_END_DECLS

#endif /* INTERFACE_H */