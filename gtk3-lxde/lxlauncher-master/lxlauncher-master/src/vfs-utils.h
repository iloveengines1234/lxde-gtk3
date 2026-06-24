/*
 *      vfs-utils.h
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

#ifndef LXLAUNCHER_VFS_UTILS_H
#define LXLAUNCHER_VFS_UTILS_H

#include <gtk/gtk.h>
#include <gio/gio.h>

G_BEGIN_DECLS

/* --- UI Asset Loader Subsystem --- */

/**
 * vfs_load_icon:
 * @theme: The active #GtkIconTheme structure mapping current desktop icons.
 * @icon_name: Name of the desired icon asset.
 * @size: Target display resolution dimension square size.
 *
 * Returns: (transfer full): A newly allocated #GdkPixbuf graphic handle, 
 * or %NULL if asset collection failed.
 */
G_GNUC_WARN_UNUSED_RESULT
GdkPixbuf* vfs_load_icon(GtkIconTheme *theme, const gchar *icon_name, gint size);


/* --- Secure Privileged Command Execution Subsystem --- */

/**
 * vfs_sudo_cmd_sync:
 * @cwd: (nullable): The absolute path working directory context, or %NULL.
 * @cmd: A single command string to execute via elevated privilege wrapper.
 * @exit_status: (out): Destination variable pointer to store the native GLib process exit code.
 * @pstdout: (out) (nullable): Destination pointer to capture standard output data.
 * @pstderr: (out) (nullable): Destination pointer to capture standard error data.
 * @error: Hook to capture underlying initialization or allocation runtime failures.
 *
 * Synchronously runs a command string under elevated privilege rings.
 *
 * Returns: TRUE if launch succeeded, FALSE if execution errors occurred.
 */
G_GNUC_WARN_UNUSED_RESULT
gboolean vfs_sudo_cmd_sync(const gchar *cwd,
                           gchar *cmd,
                           gint *exit_status,
                           gchar **pstdout,
                           gchar **pstderr,
                           GError **error);

/**
 * vfs_sudo_cmd_async:
 * @cwd: (nullable): The absolute path working directory context, or %NULL.
 * @cmd: A single command string to execute via elevated privilege wrapper.
 * @error: Hook to capture early-stage initialization failures.
 *
 * Asynchronously spawns a privileged command using the preferred escalation tool.
 *
 * Returns: TRUE if the command launch request was issued successfully, FALSE otherwise.
 */
G_GNUC_WARN_UNUSED_RESULT
gboolean vfs_sudo_cmd_async(const gchar *cwd,
                            gchar *cmd,
                            GError **error);

G_END_DECLS

#endif /* LXLAUNCHER_VFS_UTILS_H */