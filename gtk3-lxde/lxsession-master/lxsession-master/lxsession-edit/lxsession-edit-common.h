/*
 *      lxsession-edit-common.h
 *
 *      Copyright 2008 PCMan <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifndef __LXSESSION_EDIT_COMMON_H__
#define __LXSESSION_EDIT_COMMON_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* * Core autostart tracking routines and layout builders
 * Compatible with GTK 3.24.52 target API rules.
 */

void load_autostart(const char* session_name);
void save_autostart(const char* session_name);
void init_list_view(GtkTreeView* view);

/* Explicit void parameter matching strict C compilation prototypes */
GtkListStore* get_autostart_list(void);

/* UI Component signal callbacks */
void on_enable_toggled(GtkCellRendererToggle* render, char* tp_str, gpointer user_data);

G_END_DECLS

#endif /* __LXSESSION_EDIT_COMMON_H__ */