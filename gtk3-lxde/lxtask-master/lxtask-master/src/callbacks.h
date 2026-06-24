/*
 * xfce4-taskmanager - very simple taskmanger
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
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
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CALLBACKS_H_
#define __CALLBACKS_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* --- Standard Action Callbacks --- */

/**
 * on_main_button_clicked:
 * @button: The GtkButton instance that triggered the activation event.
 * @user_data: Application context pointer.
 *
 * Modernized replacement for button press interceptors to follow standard 
 * GTK3 activation routing patterns.
 */
void on_main_button_clicked(GtkButton *button, gpointer user_data);

/**
 * on_view_filter_toggled:
 * @toggle_button: The state-changing GtkToggleButton instance.
 * @user_data: Application context pointer.
 *
 * Safe signature mapping for toggle buttons. Stripped of legacy invalid GdkEvent structures.
 */
void on_view_filter_toggled(GtkToggleButton *toggle_button, gpointer user_data);


/* --- TreeView Input Interception --- */

/**
 * on_treeview1_button_press_event:
 * @widget: The core GtkTreeView layout container component.
 * @event: The hardware input pointer allocation layout tracking array.
 *
 * Returns: TRUE to stop event propagation if handled, FALSE to bubble upwards.
 */
gboolean on_treeview1_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

/**
 * on_treeview_popup_menu:
 * @widget: The active GtkTreeView processing the contextual hardware override event.
 * @user_data: Bound context parameters.
 */
gboolean on_treeview_popup_menu(GtkWidget *widget, gpointer user_data);


/* --- Context Menu Mechanics --- */

void handle_task_menu(GtkWidget *widget, const gchar *signal_type);
void handle_prio_menu(GtkWidget *widget, const gchar *priority_level);

void on_show_tasks_toggled(GtkCheckMenuItem *menu_item, gpointer user_data);
void on_show_cached_as_free_toggled(GtkCheckMenuItem *menu_item, gpointer user_data);


/* --- Application Lifetime Subsystem --- */

/**
 * on_quit:
 * Cleanly breaks active main loop sequences, synchronizes configurations, 
 * and closes open kernel pipeline configurations safely.
 */
void on_quit(void);

G_END_DECLS

#endif /* __CALLBACKS_H_ */