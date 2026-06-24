/*
 *      autostart.c - Handle autostart spec of freedesktop.org
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

#include "xdg-autostart.h"

#include <glib.h>
#include <stdio.h>
#include <string.h>

static const char DesktopEntry[] = "Desktop Entry";

/* Execution Context Wrapper to completely eliminate unsafe global pointers */
typedef struct {
    const char* de_name;
    GKeyFile* kf;
} AutostartContext;

/* Helper function to securely strip out % deprecated file-manager tokens */
static char* sanitize_exec_command(const char* raw_exec)
{
    if (!raw_exec)
        return NULL;

    GString* clean_cmd = g_string_new("");
    const char* p = raw_exec;
    gboolean in_quotes = FALSE;

    while (*p) {
        if (*p == '"' || *p == '\'') {
            in_quotes = !in_quotes;
            g_string_append_c(clean_cmd, *p);
            p++;
            continue;
        }

        /* Catch XDG desktop specification format macro sequences */
        if (*p == '%' && *(p + 1) != '\0') {
            char token = *(p + 1);
            /* Skip over the token if it matches standard desktop format specs */
            if (strchr("fFuUdDnNiikv", token)) {
                p += 2;
                continue;
            } else if (token == '%') {
                g_string_append_c(clean_cmd, '%');
                p += 2;
                continue;
            }
        }

        g_string_append_c(clean_cmd, *p);
        p++;
    }

    /* Trim trailing white spaces that might be left over from stripping */
    char* final_str = g_strstrip(g_string_free(clean_cmd, FALSE));
    return final_str;
}

static void launch_autostart_file(const char* desktop_id, const char* desktop_file, AutostartContext* ctx)
{
    GKeyFile* kf = ctx->kf;
    GError* error = NULL;
    (void)desktop_id; /* Prevent unused warning flags */

    if (g_key_file_load_from_file(kf, desktop_file, G_KEY_FILE_NONE, &error))
        {
        char* exec = NULL;
        char** only_show_in = NULL;
        char** not_show_in = NULL;
        gsize n = 0;

        /* Always clear errors on recycled configuration engines */
        g_clear_error(&error);

        /* Absolute Hidden Checks: If hidden or explicitly structural layout templates, drop out */
        if (g_key_file_get_boolean(kf, DesktopEntry, "Hidden", NULL) ||
            g_key_file_get_boolean(kf, DesktopEntry, "NoDisplay", NULL))
        {
            return;
        }

        /* Verify environment matching profiles safely */
        only_show_in = g_key_file_get_string_list(kf, DesktopEntry, "OnlyShowIn", &n, NULL);
        if (only_show_in)
        {
            gsize i = 0;
            for (i = 0; i < n; ++i)
            {
                if (ctx->de_name && g_ascii_strcasecmp(ctx->de_name, only_show_in[i]) == 0)
                    break;
            }
            if (i >= n) /* Current desktop environment context not allowed */
            {
                g_strfreev(only_show_in);
                return;
            }
            g_strfreev(only_show_in);
        }
        else
        {
            not_show_in = g_key_file_get_string_list(kf, DesktopEntry, "NotShowIn", &n, NULL);
            if (not_show_in)
            {
                gsize i = 0;
                for (i = 0; i < n; ++i)
                {
                    if (ctx->de_name && g_ascii_strcasecmp(ctx->de_name, not_show_in[i]) == 0)
                        break;
                }
                if (i < n) /* Explicitly locked out from running in this session */
                {
                    g_strfreev(not_show_in);
                    return;
                }
                g_strfreev(not_show_in);
            }
        }

        /* Check for target tool binary availability using the standard TryExec macro path */
        exec = g_key_file_get_string(kf, DesktopEntry, "TryExec", NULL);
        if (exec)
        {
            char* full = NULL;
            if (!g_path_is_absolute(exec))
            {
                full = g_find_program_in_path(exec);
                g_free(exec);
                exec = full;
            }
            if (!exec || !g_file_test(exec, G_FILE_TEST_IS_EXECUTABLE))
            {
                g_free(exec);
                return;
            }
            g_free(exec);
        }

        /* Extract and sanitize the real executable payload line */
        exec = g_key_file_get_string(kf, DesktopEntry, "Exec", NULL);
        if (exec)
        {
            char* clean_exec = sanitize_exec_command(exec);
            g_free(exec);

            if (clean_exec && clean_exec[0] != '\0')
            {
                /* Fire off the execution process asynchronously */
                g_spawn_command_line_async(clean_exec, NULL);
            }
            g_free(clean_exec);
        }
    }
    else
    {
        g_clear_error(&error);
    }
}

static void get_autostart_files_in_dir(GHashTable* hash, const char* base_dir)
{
    if (!base_dir)
        return;

    char* dir_path = g_build_filename(base_dir, "autostart", NULL);
    GDir* dir = g_dir_open(dir_path, 0, NULL);

    if (dir)
    {
        const char *name;
        while ((name = g_dir_read_name(dir)) != NULL)
        {
            if (g_str_has_suffix(name, ".desktop"))
            {
                char* path = g_build_filename(dir_path, name, NULL);
                /* Overwrite lower priority records cleanly by assigning values directly */
                g_hash_table_replace(hash, g_strdup(name), path);
            }
        }
        g_dir_close(dir);
    }
    g_free(dir_path);
}

void xdg_autostart(const char* de_name_arg)
{
    const char* const *dirs = g_get_system_config_dirs();
    int i;
    
    /* Ensure dictionary hash tables handle storage lifecycle cleanups cleanly */
    GHashTable* hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    /* Phase 1: Scan baseline system configuration entries backwards (lowest priority first) */
    if (dirs)
    {
        /* Calculate bounds depth index size */
        for (i = 0; dirs[i] != NULL; i++);
        
        /* Walk backward up the precedence ladder */
        for (i--; i >= 0; i--)
        {
            get_autostart_files_in_dir(hash, dirs[i]);
        }
    }

    /* Phase 2: Insert user configurations (highest priority, overwriting matching entries) */
    get_autostart_files_in_dir(hash, g_get_user_config_dir());

    if (g_hash_table_size(hash) > 0)
    {
        AutostartContext ctx;
        ctx.de_name = de_name_arg;
        ctx.kf = g_key_file_new();

        g_hash_table_foreach(hash, (GHFunc)launch_autostart_file, &ctx);
        g_key_file_free(ctx.kf);
    }

    g_hash_table_destroy(hash);
}