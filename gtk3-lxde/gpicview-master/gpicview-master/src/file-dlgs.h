/*
 *      file-dlgs.h
 *
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

#ifndef _FILE_DLGS_H_
#define _FILE_DLGS_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

/**
 * get_open_filename:
 * @parent: The parent GtkWindow for transient positioning.
 * @cwd: The initial directory path, or NULL.
 *
 * Opens a native GTK 3 file chooser dialog in open mode.
 *
 * Returns: A newly allocated string containing the selected file path,
 * or NULL if canceled. Free with g_free() when done.
 */
char* get_open_filename(GtkWindow* parent, const char* cwd);

/**
 * get_save_filename:
 * @parent: The parent GtkWindow for transient positioning.
 * @cwd: The initial directory path, or NULL.
 * @type: Out parameter populated with the g_object_get_data target type key.
 *
 * Opens a native GTK 3 file chooser dialog in save mode with dynamic extension filters.
 *
 * Returns: A newly allocated string containing the destination path,
 * or NULL if canceled. Free with g_free() when done.
 */
char* get_save_filename(GtkWindow* parent, const char* cwd, char** type);

G_END_DECLS

#endif /* _FILE_DLGS_H_ */