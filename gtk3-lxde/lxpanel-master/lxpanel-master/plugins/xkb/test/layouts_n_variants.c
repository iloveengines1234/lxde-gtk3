
// gcc layouts_n_variants.c -o layouts_n_variants `pkg-config --cflags --libs glib-2.0`

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>


/**
 * Query the XKB base directory using pkg-config
 *
 * @return Allocated string containing the XKB directory path, or NULL if not found.
 *         Must be freed with g_free().
 */
static gchar *
query_xkb_directory(void)
{
    GError *error = NULL;
    gchar *stdout_text = NULL;
    gchar **lines = NULL;
    gchar *xkb_dir = NULL;

    const gchar *command = "pkg-config --variable=xkb_base xkeyboard-config";

    /* Execute the command and capture output */
    if (!g_spawn_command_line_sync(command, &stdout_text, NULL, NULL, &error))
    {
        g_warning("Failed to run command: %s", error->message);
        g_error_free(error);
        return NULL;
    }

    if (stdout_text == NULL || *stdout_text == '\0')
    {
        g_free(stdout_text);
        return NULL;
    }

    /* Split output into lines */
    lines = g_strsplit(stdout_text, "\n", -1);
    g_free(stdout_text);

    /* Find the first line that starts with '/' */
    for (gint i = 0; lines[i] != NULL; i++)
    {
        gchar *line = g_strstrip(lines[i]);
        if (line[0] == '/')
        {
            xkb_dir = g_strdup(line);
            break;
        }
    }

    g_strfreev(lines);
    return xkb_dir;
}

int
main(int argc, char *argv[])
{
    (void)argc;  /* Suppress unused parameter warnings */
    (void)argv;

    g_autofree gchar *xkb_dir = query_xkb_directory();

    g_message("Found: '%s'", xkb_dir != NULL ? xkb_dir : "none");

    return EXIT_SUCCESS;
}
