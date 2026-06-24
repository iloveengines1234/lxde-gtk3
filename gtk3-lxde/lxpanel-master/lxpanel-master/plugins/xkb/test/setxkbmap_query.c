
// gcc setxkbmap_query.c -o setxkbmap_query `pkg-config --cflags --libs glib-2.0`

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
#include <glib.h>


typedef struct
{
    const gchar *label;
    const gchar *pattern;
} XkbPattern;

/**
 * Extract XKB configuration from setxkbmap output
 *
 * Queries setxkbmap and extracts model, layout, and variant settings.
 *
 * @return TRUE if successful, FALSE on error
 */
static gboolean
query_xkb_settings(void)
{
    GError *error = NULL;
    g_autofree gchar *stdout_text = NULL;
    g_auto(GStrv) lines = NULL;

    const XkbPattern patterns[] = {
        {"model", "(?<=model:).*"},
        {"layout", "(?<=layout:).*"},
        {"variant", "(?<=variant:).*"},
        {NULL, NULL}
    };

    /* Execute setxkbmap -query */
    if (!g_spawn_command_line_sync("setxkbmap -query", &stdout_text, NULL, NULL, &error))
    {
        g_warning("Failed to run setxkbmap: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    if (stdout_text == NULL || *stdout_text == '\0')
    {
        g_message("No output from setxkbmap");
        return FALSE;
    }

    /* Split output into lines */
    lines = g_strsplit(stdout_text, "\n", -1);

    /* Compile regex patterns */
    g_autoptr(GRegex) *regexes = g_new0(GRegex *, 3);
    for (gint i = 0; patterns[i].label != NULL; i++)
    {
        regexes[i] = g_regex_new(patterns[i].pattern, 0, 0, &error);
        if (regexes[i] == NULL)
        {
            g_warning("Failed to compile regex '%s': %s",
                      patterns[i].pattern, error->message);
            g_error_free(error);
            g_free(regexes);
            return FALSE;
        }
    }

    /* Process each line */
    for (gint line_idx = 0; lines[line_idx] != NULL; line_idx++)
    {
        for (gint pattern_idx = 0; patterns[pattern_idx].label != NULL; pattern_idx++)
        {
            g_autoptr(GMatchInfo) match_info = NULL;

            if (g_regex_match(regexes[pattern_idx], lines[line_idx], 0, &match_info))
            {
                g_autofree gchar *value = g_match_info_fetch(match_info, 0);
                if (value != NULL)
                {
                    gchar *trimmed = g_strchug(value);
                    g_message("Found %s: '%s'", patterns[pattern_idx].label, trimmed);
                }
            }
        }
    }

    /* Cleanup regexes */
    for (gint i = 0; patterns[i].label != NULL; i++)
    {
        g_regex_unref(regexes[i]);
    }
    g_free(regexes);

    return TRUE;
}

int
main(int argc, char *argv[])
{
    (void)argc;  /* Suppress unused parameter warnings */
    (void)argv;

    gboolean success = query_xkb_settings();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
