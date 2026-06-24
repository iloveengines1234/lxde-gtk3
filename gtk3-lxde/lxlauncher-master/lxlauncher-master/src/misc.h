/*
 *      misc.h
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

#ifndef LXLAUNCHER_MISC_H
#define LXLAUNCHER_MISC_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* --- Application Launch Subsystem --- */

/**
 * lxlauncher_execute_app:
 * @screen: (nullable): A #GdkScreen instance to execute the application on, or %NULL to use the default screen.
 * @working_dir: (nullable): Absolute path to the execution working directory, or %NULL.
 * @desktop_entry_exec: The raw "Exec" line command string extracted out of a .desktop file.
 * @app_disp_name: (nullable): The user-visible display name of the target application.
 * @file_paths: (nullable) (element-type utf8): A #GList containing strings of file paths to pass to the executed application, or %NULL.
 * @in_terminal: If TRUE, wraps the command pipeline to force execution inside a system terminal emulator.
 * @error: Location to store a #GError on failure.
 *
 * Safe wrapper engine designed to parse standard Freedesktop .desktop command keys, append payload 
 * target files, and spawn the resulting child binary safely on X11.
 *
 * Returns: TRUE if execution was launched successfully, FALSE if runtime errors were encountered.
 */
G_GNUC_WARN_UNUSED_RESULT
gboolean lxlauncher_execute_app(GdkScreen *screen,
                                const gchar *working_dir,
                                const gchar *desktop_entry_exec,
                                const gchar *app_disp_name,
                                GList *file_paths,
                                gboolean in_terminal,
                                GError **error);


/* --- UI Graphical Asset Management --- */

/**
 * lxlauncher_load_icon:
 * @name: Absolute path to an icon file, theme icon name, or icon resource string.
 * @size: Target pixel size for the loaded icon.
 * @use_fallback: If TRUE, fall back to a generic icon when the requested icon cannot be found.
 *
 * Resolves an icon entry from an absolute file path, standard icon theme, or legacy pixmap directories.
 * Returns a new #GdkPixbuf instance on success, or %NULL when no valid icon could be loaded.
 */
G_GNUC_WARN_UNUSED_RESULT
GdkPixbuf* lxlauncher_load_icon(const gchar *name, gint size, gboolean use_fallback);

G_END_DECLS

#endif /* LXLAUNCHER_MISC_H */