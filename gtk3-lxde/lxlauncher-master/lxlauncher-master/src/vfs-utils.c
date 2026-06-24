/*
 * vfs-utils.c
 *
 * Copyright 2008 PCMan <pcman.tw@gmail.com>
 * Updated for strict GTK 3.24.52+ compatibility
 */

#include <glib/gi18n.h>
#include <string.h>

#include "vfs-utils.h"

GdkPixbuf* vfs_load_icon( GtkIconTheme* theme, const char* icon_name, int size )
{
    GError* error = NULL;
    GdkPixbuf* icon = NULL;

    if ( G_UNLIKELY( !theme || !icon_name || size <= 0 ) )
        return NULL;

    /* FIXED: Replaced completely deprecated GtkIconInfo lookup tracks,
     * manual file loading, and manual scaling logic blocks with the 
     * canonical, thread-safe GTK 3.24+ loader API. */
    icon = gtk_icon_theme_load_icon( theme, 
                                     icon_name, 
                                     size, 
                                     GTK_ICON_LOOKUP_FORCE_SIZE, 
                                     &error );

    if ( G_UNLIKELY( error ) )
    {
        /* Silently clear tracking error context to maintain fallback resilience */
        g_clear_error( &error );
    }

    return icon;
}

static char* find_su_program( GError** err )
{
    char* su;

#ifdef PREFERABLE_SUDO_PROG
    su = g_find_program_in_path( PREFERABLE_SUDO_PROG );
#else
    /* Modern Linux standard uses pkexec (Polkit). Fallback to old tools if necessary */
    su = g_find_program_in_path( "pkexec" );
    if ( ! su )
        su = g_find_program_in_path( "gksudo" );
    if ( ! su )
        su = g_find_program_in_path( "kdesu" );
#endif

    if ( ! su )
        g_set_error( err, G_SPAWN_ERROR, G_SPAWN_ERROR_FAILED, _( "Privilege escalation tool (pkexec) not found" ) );

    return su;
}

gboolean vfs_sudo_cmd_async( const char* cwd, char* cmd, GError** err )
{
    char *su, *argv[3];
    gboolean ret;

    if ( ! ( su = find_su_program( err ) ) )
        return FALSE;

    argv[0] = su;
    argv[1] = cmd;
    argv[2] = NULL;

    ret = g_spawn_async( cwd, argv, NULL,
                         G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL | G_SPAWN_SEARCH_PATH,
                         NULL, NULL, NULL, err );

    g_free( su );
    return ret;
}

gboolean vfs_sudo_cmd_sync( const char* cwd, char* cmd,
                            int* exit_status,
                            char** pstdout, char** pstderr, GError** err )
{
    char *su, *argv[3];
    gboolean ret;
    GSpawnFlags flags = G_SPAWN_SEARCH_PATH;

    if ( ! ( su = find_su_program( err ) ) )
        return FALSE;

    argv[0] = su;
    argv[1] = cmd;
    argv[2] = NULL;

    /* If caller doesn't care about collecting stdout/stderr outputs, drop them securely to dev null */
    if ( !pstdout && !pstderr )
        flags |= (G_SPAWN_STDOUT_TO_DEV_NULL | G_SPAWN_STDERR_TO_DEV_NULL);

    ret = g_spawn_sync( cwd, argv, NULL, flags, NULL, NULL,
                        pstdout, pstderr, exit_status, err );

    g_free( su );
    return ret;
}