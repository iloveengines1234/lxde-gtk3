/*
 * misc.c
 * * Copyright 2008 PCMan <pcman@debian>
 * * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "misc.h"
#include "vfs-utils.h"
#include "vfs-execute.h"

static GdkPixbuf* load_icon_file( const char* file_name, int size )
{
    GdkPixbuf* icon = NULL;
    char* file_path;
    const gchar** dirs = (const gchar**) g_get_system_data_dirs();
    const gchar** dir;

    for( dir = dirs; *dir; ++dir )
    {
        file_path = g_build_filename( *dir, "pixmaps", file_name, NULL );
        icon = gdk_pixbuf_new_from_file_at_scale( file_path, size, size, TRUE, NULL );
        g_free( file_path );
        if( icon )
            break;
    }
    return icon;
}

GdkPixbuf* lxlauncher_load_icon( const char* name, int size, gboolean use_fallback )
{
    GtkIconTheme* theme;
    char *icon_name = NULL;
    const char *suffix;
    GdkPixbuf* icon = NULL;

    if( name )
    {
        if( g_path_is_absolute( name ) )
        {
            icon = gdk_pixbuf_new_from_file_at_scale( name, size, size, TRUE, NULL );
        }
        else
        {
            theme = gtk_icon_theme_get_default();
            suffix = strrchr( name, '.' );
            if( suffix
                && (0 == g_ascii_strcasecmp(suffix + 1, "png")
                || 0 == g_ascii_strcasecmp(suffix + 1, "svg")
                || 0 == g_ascii_strcasecmp(suffix + 1, "xpm")) )
            {
                /* Try to find it inside standard layout pixmaps dirs */
                icon = load_icon_file( name, size );
                if( G_UNLIKELY( ! icon ) )
                {
                    /* Strip the extension to attempt looking up inside current active Icon Theme */
                    icon_name = g_strndup( name, (suffix - name) );
                    icon = vfs_load_icon( theme, icon_name, size );
                    g_free( icon_name );
                }
            }
            else
            {
                icon = vfs_load_icon( theme, name, size );
            }
        }
    }

    if( G_UNLIKELY( ! icon ) && use_fallback )
    {
        theme = gtk_icon_theme_get_default();
        icon = vfs_load_icon( theme, "application-x-executable", size );
        if( G_UNLIKELY( ! icon ) )
        {
            icon = vfs_load_icon( theme, "gnome-mime-application-x-executable", size );
        }
    }
    return icon;
}

static gboolean can_desktop_entry_open_multiple_files( const char* exec )
{
    const char* p;
    if( !exec )
        return FALSE;

    for( p = exec; *p; ++p )
    {
        if( *p == '%' )
        {
            ++p;
            switch( *p )
            {
            case 'U':
            case 'F':
            case 'N':
            case 'D':
                return TRUE;
            case '\0':
                return FALSE;
            }
        }
    }
    return FALSE;
}

/*
* Parses Exec token macros matching the XDG Desktop Entry Specification
*/
static char* translate_app_exec_to_command_line( const char* pexec, GList* file_list )
{
    char* file;
    GList* l;
    gchar *tmp;
    GString* cmd = g_string_new("");
    gboolean add_files = FALSE;

    for( ; *pexec; ++pexec )
    {
        if( *pexec == '%' )
        {
            ++pexec;
            switch( *pexec )
            {
            case 'U':
                for( l = file_list; l; l = l->next )
                {
                    tmp = g_filename_to_uri( (char*)l->data, NULL, NULL );
                    file = g_shell_quote( tmp );
                    g_free( tmp );
                    g_string_append( cmd, file );
                    g_string_append_c( cmd, ' ' );
                    g_free( file );
                }
                add_files = TRUE;
                break;
            case 'u':
                if( file_list && file_list->data )
                {
                    tmp = g_filename_to_uri( (char*)file_list->data, NULL, NULL );
                    file = g_shell_quote( tmp );
                    g_free( tmp );
                    g_string_append( cmd, file );
                    g_free( file );
                    add_files = TRUE;
                }
                break;
            case 'F':
            case 'N':
                for( l = file_list; l; l = l->next )
                {
                    file = (char*)l->data;
                    tmp = g_shell_quote( file );
                    g_string_append( cmd, tmp );
                    g_string_append_c( cmd, ' ' );
                    g_free( tmp );
                }
                add_files = TRUE;
                break;
            case 'f':
            case 'n':
                if( file_list && file_list->data )
                {
                    file = (char*)file_list->data;
                    tmp = g_shell_quote( file );
                    g_string_append( cmd, tmp );
                    g_free( tmp );
                    add_files = TRUE;
                }
                break;
            case 'D':
                for( l = file_list; l; l = l->next )
                {
                    tmp = g_path_get_dirname( (char*)l->data );
                    file = g_shell_quote( tmp );
                    g_free( tmp );
                    g_string_append( cmd, file );
                    g_string_append_c( cmd, ' ' );
                    g_free( file );
                }
                add_files = TRUE;
                break;
            case 'd':
                if( file_list && file_list->data )
                {
                    tmp = g_path_get_dirname( (char*)file_list->data );
                    file = g_shell_quote( tmp );
                    g_free( tmp );
                    g_string_append( cmd, file );
                    g_free( file );
                    add_files = TRUE;
                }
                break;
            case 'c':
            case 'i':
            case 'k':
            case 'v':
                /* Unsupported spec fields dropped silently */
                break;
            case '%':
                g_string_append_c ( cmd, '%' );
                break;
            case '\0':
                goto _finish;
            }
        }
        else
        {
            g_string_append_c ( cmd, *pexec );
        }
    }
_finish:
    if( ! add_files )
    {
        g_string_append_c ( cmd, ' ' );
        for( l = file_list; l; l = l->next )
        {
            file = (char*)l->data;
            tmp = g_shell_quote( file );
            g_string_append( cmd, tmp );
            g_string_append_c( cmd, ' ' );
            g_free( tmp );
        }
    }

    return g_string_free( cmd, FALSE );
}

gboolean lxlauncher_execute_app( GdkScreen* screen,
                                 const char* working_dir,
                                 const char* desktop_entry_exec,
                                 const char* app_disp_name,
                                 GList* file_paths,
                                 gboolean in_terminal,
                                 GError** err )
{
    char* exec = NULL;
    char* cmd;
    GList* l;
    gchar** argv = NULL;
    gint argc = 0;
    const char* sn_desc;
    gboolean ret = FALSE;

    if( !desktop_entry_exec )
    {
        g_set_error( err, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, _("Command not found") );
        return FALSE;
    }

    /* Fallback if no macros are supplied */
    if ( ! strchr( desktop_entry_exec, '%' ) )
    {
        exec = g_strconcat( desktop_entry_exec, " %f", NULL );
    }
    else
    {
        exec = g_strdup( desktop_entry_exec );
    }

    if( !screen )
        screen = gdk_screen_get_default();

    sn_desc = app_disp_name ? app_disp_name : exec;

    /* Handle multi-file capable commands (%F, %U, %N, %D) */
    if( can_desktop_entry_open_multiple_files( exec ) )
    {
        cmd = translate_app_exec_to_command_line( exec, file_paths );
        if ( cmd && cmd[0] != '\0' )
        {
            if( g_shell_parse_argv( cmd, &argc, &argv, NULL ) )
            {
                ret = vfs_exec_on_screen( screen, working_dir, argv, NULL,
                                          sn_desc, VFS_EXEC_DEFAULT_FLAGS, err );
                g_strfreev( argv );
            }
        }
        g_free( cmd );
    }
    else 
    {
        /* FIXED: Check if file_paths is empty to process the default executable string mapping */
        if( !file_paths )
        {
            cmd = translate_app_exec_to_command_line( exec, NULL );
            if ( cmd && cmd[0] != '\0' )
            {
                if( g_shell_parse_argv( cmd, &argc, &argv, NULL ) )
                {
                    ret = vfs_exec_on_screen( screen, working_dir, argv, NULL, sn_desc,
                                              G_SPAWN_SEARCH_PATH |
                                              G_SPAWN_STDOUT_TO_DEV_NULL |
                                              G_SPAWN_STDERR_TO_DEV_NULL,
                                              err );
                    g_strfreev( argv );
                }
            }
            g_free( cmd );
        }
        else
        {
            /* Target single file macros safely (%f, %u, %d) by spawning individual processes */
            for( l = file_paths; l; l = l->next )
            {
                GList single_list = { l->data, NULL, NULL };
                cmd = translate_app_exec_to_command_line( exec, &single_list );
                if ( cmd && cmd[0] != '\0' )
                {
                    if( g_shell_parse_argv( cmd, &argc, &argv, NULL ) )
                    {
                        /* FIXED: Capture loop failure states safely across iterations */
                        gboolean r = vfs_exec_on_screen( screen, working_dir, argv, NULL, sn_desc,
                                                         G_SPAWN_SEARCH_PATH |
                                                         G_SPAWN_STDOUT_TO_DEV_NULL |
                                                         G_SPAWN_STDERR_TO_DEV_NULL,
                                                         err );
                        if( r ) ret = TRUE;
                        g_strfreev( argv );
                    }
                }
                g_free( cmd );
            }
        }
    }

    g_free( exec );
    return ret; /* FIXED: Propagates actual execution runtime success state */
}