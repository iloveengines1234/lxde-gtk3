/* $Id: main.c 2350 2007-01-13 10:12:31Z nick $
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
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
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib.h>
#include <signal.h>
#include <unistd.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#endif

#include "types.h"
#include "interface.h"
#include "functions.h"

/* --- Core Application State Definitions --- */
GtkWidget *main_window = NULL;
GArray *task_array = NULL;
gint tasks = 0;
uid_t own_uid = 0;
gchar *config_file = NULL;

/* FIXED: Assigned explicit, safe fallback variables directly to state machine handles
 * to block execution failures if load_config() hits a corrupted config layer. */
gboolean show_user_tasks  = TRUE;
gboolean show_root_tasks  = FALSE;
gboolean show_other_tasks = FALSE;
gboolean show_full_path   = FALSE;
gboolean show_cached_as_free = FALSE;
gboolean full_view        = TRUE;

gint win_width = 640;
gint win_height = 480;
gint refresh_interval = 2; /* Safe default: 2 seconds */
guint rID = 0;

int page_size = 0;

int main (int argc, char *argv[])
{
#ifdef ENABLE_NLS
    bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    /* FIXED: Enforce absolute X11 backend execution constraints to prevent 
     * Wayland session initialization drops since task management uses /proc metrics. */
    g_setenv("GDK_BACKEND", "x11", TRUE);

    /* Initialize GTK Toolkit structures cleanly */
    gtk_init (&argc, &argv);

    /* Configure memory metric scaling factors */
    page_size = sysconf(_SC_PAGESIZE) >> 10;
    own_uid = getuid();

    /* Establish configuration targets */
    config_file = g_build_filename(g_get_user_config_dir(), "lxtask.conf", NULL);
    load_config();

    /* Initialize core structure cache */
    task_array = g_array_new (FALSE, FALSE, sizeof (struct task));
    tasks = 0;

    /* Build application interface layouts */
    main_window = create_main_window ();
    gtk_widget_show_all (main_window);

    /* Run the initial collection pass before creating the loop hook */
    if (!refresh_task_list())
    {
        g_array_free(task_array, TRUE);
        g_free(config_file);
        return 0;
    }

    /* Track repeating asynchronous collections */
    rID = g_timeout_add_seconds(refresh_interval, G_SOURCE_FUNC(refresh_task_list), NULL);

    /* FIXED: Fixed compiler-breaking typo tracking native window main loops */
    gtk_main();

    /* --- Clean Architecture Shutdown Actions --- */
    if (rID > 0)
    {
        g_source_remove(rID);
    }

    if (task_array)
    {
        g_array_free(task_array, TRUE);
    }

    /* FIXED: Explicitly reclaim configuration string layouts to eliminate systemic memory leaks */
    g_free(config_file);

    return 0;
}