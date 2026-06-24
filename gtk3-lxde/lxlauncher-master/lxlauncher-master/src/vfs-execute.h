/*
*  C Interface: vfs-execute
*
* Description: 
*
*
* Author: Hong Jen Yee (PCMan) <pcman.tw (AT) gmail.com>, (C) 2006
*
* Copyright: See COPYING file that comes with this distribution
*
*/

#ifndef LXLAUNCHER_VFS_EXECUTE_H
#define LXLAUNCHER_VFS_EXECUTE_H

#include <glib.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * VFS_EXEC_DEFAULT_FLAGS:
 *
 * Modern baseline fallback flags. Searches the system PATH for the binary 
 * and cleanly silences uncaptured output streams to prevent terminal noise.
 */
#define VFS_EXEC_DEFAULT_FLAGS (G_SPAWN_SEARCH_PATH | G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL)

/**
 * vfs_exec_with_context:
 * @work_dir: (nullable): Absolute path to the working directory context, or %NULL.
 * @argv: A NULL-terminated array of strings representing the program and its arguments.
 * @envp: (nullable): A NULL-terminated array of strings representing the environment, or %NULL.
 * @context: (nullable): A modern #GdkAppLaunchContext tracking startup tokens, or %NULL.
 * @flags: Bitwise flags tailoring process allocation restrictions.
 * @error: Hook to capture underlying process initialization failures.
 *
 * Spawns a child process cleanly and securely across X11 and Wayland backends.
 * Natively integrates with desktop startup notifications to manage cursor loading states.
 *
 * Returns: TRUE if execution succeeded, FALSE if launch errors were encountered.
 */
G_GNUC_WARN_UNUSED_RESULT
gboolean vfs_exec(const gchar *work_dir,
                  gchar **argv,
                  gchar *const *envp,
                  const gchar *disp_name,
                  GSpawnFlags flags,
                  GError **error);

G_GNUC_WARN_UNUSED_RESULT
gboolean vfs_exec_on_screen(GdkScreen *screen,
                             const gchar *work_dir,
                             gchar **argv,
                             gchar *const *envp,
                             const gchar *disp_name,
                             GSpawnFlags flags,
                             GError **error);

G_END_DECLS

#endif /* LXLAUNCHER_VFS_EXECUTE_H */