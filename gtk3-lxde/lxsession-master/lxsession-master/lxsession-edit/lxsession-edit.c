/*
 *      lxsession-edit.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <string.h>

#include "lxsession-edit-common.h"

#define CONFIG_FILE_NAME    "desktop.conf"

/* Structural wrapper to pass widget references safely into the asynchronous callback */
typedef struct {
    GtkWidget *dlg;
    GtkWidget *wm;
    GKeyFile *kf;
    char *session_name;
} ProgramData;

/* Standardized cleanup wrapper to prevent resource leak mutations */
static void program_data_free(ProgramData *data)
{
    if (!data)
        return;

    if (data->kf)
        g_key_file_free(data->kf);
    
    g_free(data->session_name);
    g_slice_free(ProgramData, data);
}

/* Modern asynchronous handler replacing the blocking gtk_dialog_run loop */
static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    ProgramData *data = (ProgramData *)user_data;

    if (response_id == GTK_RESPONSE_OK)
    {
        save_autostart(data->session_name);

        if (data->wm) /* if wm settings is available. */
        {
            char *dir;
            char *cfg;
            const char *wm_cmd;

            dir = g_build_filename(g_get_user_config_dir(), "lxsession", data->session_name, NULL);
            g_mkdir_with_parents(dir, 0700);
            cfg = g_build_filename(dir, "desktop.conf", NULL);
            g_free(dir);

            /* Compatible with GTK 3.24 entry allocation behaviors */
            wm_cmd = gtk_entry_get_text(GTK_ENTRY(data->wm));
            if (wm_cmd && *wm_cmd != '\0')
            {
                char *file_data;
                gsize len;
                g_key_file_set_string(data->kf, "Session", "windows_manager/command", wm_cmd);
                file_data = g_key_file_to_data(data->kf, &len, NULL);
                g_file_set_contents(cfg, file_data, len, NULL);
                g_free(file_data);
            }
            g_free(cfg);
        }
    }

    /* Tear down window frame layout cleanly */
    gtk_widget_destroy(data->dlg);
    program_data_free(data);

    /* Safely break the main cycle loops to exit the application binary */
    gtk_main_quit();
}

/* Catch explicit user panel deletion window actions to avoid leaky escapes */
static gboolean on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
    on_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CLOSE, user_data);
    return TRUE;
}

int main(int argc, char** argv)
{
    GtkBuilder *builder;
    GtkWidget *dlg, *autostarts, *wm, *adv_page, *notebook;
    GKeyFile* kf;
    char *cfg = NULL;
    char *wm_cmd = NULL;
    gboolean loaded = FALSE;
    const char* env_session = NULL;
    ProgramData *data;

#ifdef ENABLE_NLS
    bindtextdomain ( GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR );
    bind_textdomain_codeset ( GETTEXT_PACKAGE, "UTF-8" );
    textdomain ( GETTEXT_PACKAGE );
#endif

    gtk_init( &argc, &argv );
    
    data = g_slice_new0(ProgramData);

    if( argc > 1 )
        data->session_name = g_strdup(argv[1]);
    else
    {
        env_session = g_getenv("XDG_CURRENT_DESKTOP");
        if(!env_session)
        {
            env_session = g_getenv("DESKTOP_SESSION");
            if( G_UNLIKELY(!env_session) )
                env_session = "LXDE";
        }
        data->session_name = g_strdup(env_session);
    }

    builder = gtk_builder_new();
    if( !gtk_builder_add_from_file( builder, PACKAGE_UI_DIR "/lxsession-edit.ui", NULL ) )
    {
        g_free(data->session_name);
        g_slice_free(ProgramData, data);
        g_object_unref(builder);
        return 1;
    }

    dlg = (GtkWidget*) gtk_builder_get_object( builder, "dlg" );
    autostarts = (GtkWidget*) gtk_builder_get_object( builder, "autostarts" );
    adv_page = (GtkWidget*) gtk_builder_get_object( builder, "adv_page" );
    notebook = (GtkWidget*) gtk_builder_get_object( builder, "notebook" );
    wm = (GtkWidget*) gtk_builder_get_object( builder, "wm" );
    g_object_unref(builder);

    /* Set icon name for main window so it displays correctly across desktop panels. */
    gtk_window_set_icon_name(GTK_WINDOW(dlg), "preferences-desktop");

    /* Initialize autostart layout grids */
    init_list_view((GtkTreeView*)autostarts);
    load_autostart(data->session_name);
    gtk_tree_view_set_model( (GtkTreeView*)autostarts, (GtkTreeModel*)get_autostart_list() );

    kf = g_key_file_new();

    /* Check runtime environment for desktop configurations */
    if( g_getenv("_LXSESSION_PID") )
    {
        cfg = g_build_filename( g_get_user_config_dir(), "lxsession", data->session_name, CONFIG_FILE_NAME, NULL );
        loaded = g_key_file_load_from_file(kf, cfg, 0, NULL);
        g_free(cfg);

        if( !loaded )
        {
            const char* const *dirs = g_get_system_config_dirs();
            const char* const *dir;
            for( dir = dirs; *dir; ++dir )
            {
                cfg = g_build_filename( *dir, "lxsession", data->session_name, CONFIG_FILE_NAME, NULL );
                loaded = g_key_file_load_from_file(kf, cfg, 0, NULL);
                g_free( cfg );
                if( loaded )
                    break;
            }
        }
        if( loaded )
            wm_cmd = g_key_file_get_string(kf, "Session", "windows_manager/command", NULL);

        if( ! wm_cmd || !*wm_cmd )
        {
            g_free(wm_cmd);
            if( strcmp(data->session_name, "LXDE") == 0 )
                wm_cmd = g_strdup("openbox-lxde");
            else
                wm_cmd = g_strdup("openbox");
        }
        gtk_entry_set_text((GtkEntry*)wm, wm_cmd);
        g_free(wm_cmd);
    }
    else
    {
        /* Safe separation of layout components utilizing GTK 3.24 structure logic */
        if (GTK_IS_NOTEBOOK(notebook) && GTK_IS_WIDGET(adv_page))
        {
            gint page_num = gtk_notebook_page_num(GTK_NOTEBOOK(notebook), adv_page);
            if (page_num >= 0)
                gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), page_num);
        }
        else if (GTK_IS_WIDGET(adv_page))
        {
            gtk_widget_destroy(adv_page);
        }
        wm = adv_page = NULL;
    }

    /* Package reference targets inside structural wrapper */
    data->dlg = dlg;
    data->wm = wm;
    data->kf = kf;

    /* Attach asynchronous listener response to process inputs cleanly */
    g_signal_connect(dlg, "response", G_CALLBACK(on_dialog_response), data);
    g_signal_connect(dlg, "delete-event", G_CALLBACK(on_delete_event), data);
    
    /* Ensure widgets render visible layouts and spin up execution thread loops */
    gtk_widget_show_all(dlg);
    gtk_main();

    return 0;
}