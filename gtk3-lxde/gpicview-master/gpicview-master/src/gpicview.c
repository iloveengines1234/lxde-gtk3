/***************************************************************************
 *   Copyright (C) 2007 by PCMan (Hong Jen Yee) <pcman.tw@gmail.com>       *
 *   Copyright (C) 2023 by Ingo Brückl                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>

#include "pref.h"
#include "main-win.h"

static char** files = NULL;
static gboolean should_display_version = FALSE;
static gboolean should_start_slideshow = FALSE;

// GOptionEntry remains mostly the same, but handled via GApplication now
static GOptionEntry opt_entries[] =
{
    {G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &files, NULL, N_("[FILE]")},
    {"version", 'v', G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &should_display_version,
                 N_("Print version information and exit"), NULL },
    {"slideshow", 0, G_OPTION_FLAG_IN_MAIN, G_OPTION_ARG_NONE, &should_start_slideshow,
                 N_("Start slideshow"), NULL },
    { NULL }
};

#define PIXMAP_DIR     PACKAGE_DATA_DIR "/gpicview/pixmaps/"

static void activate(GtkApplication *app, gpointer user_data)
{
    MainWin* win;

    load_preferences();

    /* Allocate and show the window using GTK3 mechanisms */
    win = (MainWin*)main_win_new();
    gtk_application_add_window(app, GTK_WINDOW(win));
    gtk_widget_show_all(GTK_WIDGET(win)); // GTK3 prefers show_all for containers/windows

    if (pref.open_maximized)
        gtk_window_maximize(GTK_WINDOW(win));

    // FIXME: need to process multiple files...
    if( files )
    {
        if( G_UNLIKELY( *files[0] != '/' && strstr( files[0], ":/" )) )    // This is an URI
        {
            char* path = g_filename_from_uri( files[0], NULL, NULL );

            if ( !path )
            {
                gchar *msg = g_strdup_printf( _("The file \"%s\" cannot be opened!"), files[0] );
                main_win_show_error( win, msg );
                g_free( msg );
                return;
            }

            main_win_open( win, path, ZOOM_NONE );
            g_free( path );
        }
        else
            main_win_open( win, files[0], ZOOM_NONE );

        if (should_start_slideshow)
            main_win_start_slideshow ( win );
    }
    else
    {
        main_win_open( win, ".", ZOOM_NONE );
    }
}

int main(int argc, char *argv[])
{
    GtkApplication *app;
    int status;

#ifdef ENABLE_NLS
    bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
    textdomain ( GETTEXT_PACKAGE );
#endif

    // Create a modern GtkApplication instance
    app = gtk_application_new("org.lxde.gpicview", G_APPLICATION_HANDLES_COMMAND_LINE);

    // Modern way to handle command line arguments in GTK3
    g_application_add_main_option_entries(G_APPLICATION(app), opt_entries);

    // Connect the activation signal where the window is created
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);

    // Run the application (this replaces gtk_main and handles option parsing implicitly)
    status = g_application_run(G_APPLICATION(app), argc, argv);
    
    // Check for explicit version flag call before cleaning up
    if( should_display_version )
    {
        printf( "gpicview %s\n", VERSION );
        g_object_unref(app);
        return 0;
    }

    // Modern icon theme handling
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), PIXMAP_DIR);

    save_preferences();
    g_object_unref(app);

    return status;
}