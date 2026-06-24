/*
* C Implementation: vfs-execute
*
* Description: Cleaned and updated for modern strict GTK3 compilation
*
* Author: Hong Jen Yee (PCMan) <pcman.tw (AT) gmail.com>, (C) 2006
* Copyright: See COPYING file that comes with this distribution
*/ 

#include "vfs-execute.h"

#define SN_API_NOT_YET_FROZEN
#include <libsn/sn-launcher.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>  /* CRITICAL: Required for modern explicit X11 layer interaction */
#include <X11/Xatom.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>

extern char **environ;

gboolean vfs_exec( const char* work_dir,
                   char** argv, char** envp,
                   const char* disp_name,
                   GSpawnFlags flags,
                   GError **err )
{
    return vfs_exec_on_screen( gdk_screen_get_default(), work_dir,
                               argv, envp, disp_name, flags, err );
}

static gboolean sn_timeout( gpointer user_data )
{
    SnLauncherContext * ctx = ( SnLauncherContext* ) user_data;
    
    sn_launcher_context_complete ( ctx );
    sn_launcher_context_unref ( ctx );
    
    return FALSE;
}

static gint tvsn_get_active_workspace_number ( GdkScreen *screen )
{
    GdkWindow * root;
    gulong bytes_after_ret = 0;
    gulong nitems_ret = 0;
    guint *prop_ret = NULL;
    Atom _NET_CURRENT_DESKTOP;
    Atom _WIN_WORKSPACE;
    Atom type_ret = None;
    gint format_ret;
    gint ws_num = 0;

    gdk_error_trap_push ();

    root = gdk_screen_get_root_window ( screen );
    if (!root) {
        gdk_error_trap_pop_ignored();
        return 0;
    }

    /* FIXED: Swapped out non-existent legacy macros for modern GTK3 equivalents */
    Display* xdisplay = gdk_x11_window_get_xdisplay ( root );
    Window xroot_id = gdk_x11_window_get_xid ( root );

    _NET_CURRENT_DESKTOP = XInternAtom ( xdisplay, "_NET_CURRENT_DESKTOP", False );
    _WIN_WORKSPACE = XInternAtom ( xdisplay, "_WIN_WORKSPACE", False );

    if ( XGetWindowProperty ( xdisplay, xroot_id, _NET_CURRENT_DESKTOP, 0, 32, 
                              False, XA_CARDINAL, &type_ret, &format_ret, 
                              &nitems_ret, &bytes_after_ret, ( gpointer ) &prop_ret ) != Success )
    {
        if ( XGetWindowProperty ( xdisplay, xroot_id, _WIN_WORKSPACE, 0, 32, 
                                  False, XA_CARDINAL, &type_ret, &format_ret, 
                                  &nitems_ret, &bytes_after_ret, ( gpointer ) &prop_ret ) != Success )
        {
            if ( prop_ret != NULL )
            {
                XFree ( prop_ret );
                prop_ret = NULL;
            }
        }
    }

    if ( prop_ret != NULL )
    {
        if ( type_ret != None && format_ret != 0 )
            ws_num = (gint)*prop_ret;
        XFree ( prop_ret );
    }

    gdk_error_trap_pop_ignored ();
    return ws_num;
}

/* Custom wrapper matching SnDisplay expects to ignore pop return integer securely */
static void local_sn_error_trap_pop(Display *xdisplay)
{
    (void)xdisplay;
    gdk_error_trap_pop_ignored();
}

gboolean vfs_exec_on_screen( GdkScreen* screen,
                             const char* work_dir,
                             char** argv, char** envp,
                             const char* disp_name,
                             GSpawnFlags flags,
                             GError **err )
{
    SnLauncherContext * ctx = NULL;
    SnDisplay* display;
    gboolean ret;
    char** new_env = NULL;
    int i, env_idx = 0;
    int n_env = 0;
    char* display_name;
    int display_index = -1, startup_id_index = -1;

    if ( ! envp )
        envp = environ;

    n_env = g_strv_length(envp);

    /* Allocate cleanly: room for elements + slots for DISPLAY, STARTUP_ID, and NULL-terminator */
    new_env = g_new0( char*, n_env + 3 );
    
    for ( i = 0; i < n_env; ++i )
    {
        if ( 0 == strncmp( envp[ i ], "DISPLAY=", 8 ) )
        {
            display_index = env_idx;
            new_env[env_idx++] = NULL; 
        }
        else if ( 0 == strncmp( envp[ i ], "DESKTOP_STARTUP_ID=", 19 ) )
        {
            startup_id_index = env_idx;
            new_env[env_idx++] = NULL;
        }
        else
        {
            new_env[env_idx++] = g_strdup( envp[ i ] );
        }
    }

    /* FIXED: Extracted structural XDisplay pointer from the screen surface cleanly */
    display = sn_display_new ( gdk_x11_display_get_xdisplay(gdk_screen_get_display(screen)),
                               ( SnDisplayErrorTrapPush ) gdk_error_trap_push,
                               ( SnDisplayErrorTrapPop ) local_sn_error_trap_pop );
                               
    if ( G_LIKELY ( display ) )
    {
        if ( !disp_name )
            disp_name = argv[ 0 ];

        ctx = sn_launcher_context_new( display, gdk_screen_get_number( screen ) );

        sn_launcher_context_set_description( ctx, disp_name );
        sn_launcher_context_set_name( ctx, g_get_prgname() );
        sn_launcher_context_set_binary_name( ctx, argv[ 0 ] );
        sn_launcher_context_set_workspace ( ctx, tvsn_get_active_workspace_number( screen ) );

        sn_launcher_context_initiate( ctx, g_get_prgname(), argv[ 0 ], 
                                      gtk_get_current_event_time() );

        if ( startup_id_index < 0 )
            startup_id_index = env_idx++;

        new_env[ startup_id_index ] = g_strconcat( "DESKTOP_STARTUP_ID=",
                                                   sn_launcher_context_get_startup_id ( ctx ), NULL );
    }

    display_name = gdk_screen_make_display_name ( screen );
    if ( display_index < 0 )
        display_index = env_idx++;

    new_env[ display_index ] = g_strconcat( "DISPLAY=", display_name, NULL );
    g_free( display_name );

    /* Guard and finalize explicit terminal sequence bounds */
    new_env[ env_idx ] = NULL;

    ret = g_spawn_async( work_dir, argv, new_env, flags, NULL, NULL, NULL, err );

    g_strfreev( new_env );

    if ( G_LIKELY ( ctx ) )
    {
        if ( G_LIKELY ( ret ) )
            g_timeout_add ( 20 * 1000, sn_timeout, ctx );
        else
        {
            sn_launcher_context_complete ( ctx );
            sn_launcher_context_unref ( ctx );
        }
    }

    if ( G_LIKELY ( display ) )
        sn_display_unref ( display );

    return ret;
}