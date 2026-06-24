/*
 *      main.c : the main() function for menu-cache-gen binary.
 *
 *      Copyright 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of libmenu-cache package and created program
 *      should be not used without the library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "menu-tags.h"
#include <string.h>
#include <locale.h>

/* --- Global Environment Contexts --- */
char **languages = NULL;
GSList *MenuFiles = NULL;
gint verbose = 0;

static gboolean option_verbose(const gchar *option_name, const gchar *value,
                               gpointer data, GError **error)
{
    verbose++;
    return TRUE;
}

/* --- GLib Engine Command Parsing Configuration --- */
static char *ifile = NULL;
static char *ofile = NULL;
static char *lang = NULL;

GOptionEntry opt_entries[] =
{
    { "input", 'i', 0, G_OPTION_ARG_FILENAME, &ifile, "Source *.menu file to read", "FILENAME" },
    { "output", 'o', 0, G_OPTION_ARG_FILENAME, &ofile, "Output file to write cache to", "FILENAME" },
    { "lang", 'l', 0, G_OPTION_ARG_STRING, &lang, "Language", "LANG_LIST" },
    { "verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, &option_verbose, "Send debug messages to terminal", NULL },
    { NULL }
};

int main(int argc, char **argv)
{
    FmXmlFile *xmlfile = NULL;
    GOptionContext *opt_ctx;
    GError *err = NULL;
    MenuMenu *menu;
    int rc = 1;
    gboolean with_hidden = FALSE;
    gboolean free_languages = FALSE;

    opt_ctx = g_option_context_new("Generate cache for freedesktop.org compliant menus.");
    g_option_context_add_main_entries(opt_ctx, opt_entries, NULL);
    if (!g_option_context_parse(opt_ctx, &argc, &argv, &err))
    {
        g_printerr("menu-cache-gen: %s\n", err->message);
        g_error_free(err);
        g_option_context_free(opt_ctx);
        return 1;
    }
    g_option_context_free(opt_ctx);

    /* Process system layout locale constraints */
    if (lang == NULL || lang[0] == '\0')
    {
        languages = (char **)g_get_language_names();
        free_languages = FALSE; /* Managed statically by GLib core systems */
    }
    else
    {
        languages = g_strsplit(lang, ":", 0);
        free_languages = TRUE;
    }
    setlocale(LC_ALL, "");

    /* Validate runtime parameter arguments */
    if (ifile == NULL || ofile == NULL)
    {
        g_printerr("menu-cache-gen: failed: both input and output files must be defined.\n");
        if (free_languages) g_strfreev(languages);
        g_free(ifile); g_free(ofile); g_free(lang);
        return 1;
    }

    with_hidden = g_str_has_suffix(ifile, "+hidden");
    if (with_hidden)
    {
        size_t len = strlen(ifile);
        if (len >= 7) {
            ifile[len - 7] = '\0';
        }
    }

    if (G_LIKELY(!g_path_is_absolute(ifile)))
    {
        char *path = NULL;
        gboolean found = FALSE;
        const gchar * const *dirs = g_get_system_config_dirs();
        const char *menu_prefix = g_getenv("XDG_MENU_PREFIX");

        if (menu_prefix != NULL && menu_prefix[0] != '\0')
        {
            path = g_strconcat(menu_prefix, ifile, NULL);
            g_free(ifile);
            ifile = path;
        }

        path = g_build_filename(g_get_user_config_dir(), "menus", ifile, NULL);
        found = g_file_test(path, G_FILE_TEST_IS_REGULAR);
        
        while (!found && dirs && dirs[0] != NULL)
        {
            MenuFiles = g_slist_append(MenuFiles, (gpointer)g_intern_string(path));
            g_free(path);
            
            path = g_build_filename(dirs[0], "menus", ifile, NULL);
            found = g_file_test(path, G_FILE_TEST_IS_REGULAR);
            dirs++;
        }

        if (!found)
        {
            g_printerr("menu-cache-gen: failed: cannot find file '%s'\n", ifile);
            g_free(path);
            g_slist_free(MenuFiles);
            if (free_languages) g_strfreev(languages);
            g_free(ifile); g_free(ofile); g_free(lang);
            return 1;
        }
        g_free(ifile);
        ifile = path;
    }
    MenuFiles = g_slist_append(MenuFiles, (gpointer)g_intern_string(ifile));

#if !GLIB_CHECK_VERSION(2, 36, 0)
    g_type_init();
#endif

    /* Load, merge menu descriptor schema file, and instantiate internal objects */
    menu = get_merged_menu(ifile, &xmlfile, &err);
    if (menu == NULL)
    {
        g_printerr("menu-cache-gen: %s\n", err ? err->message : "Unknown error processing layout xml");
        if (err) g_error_free(err);
        g_slist_free(MenuFiles);
        if (free_languages) g_strfreev(languages);
        g_free(ifile); g_free(ofile); g_free(lang);
        return 1;
    }

    /* Persist tracking binary generation file assets down to storage targets */
    rc = !save_menu_cache(menu, ifile, ofile, with_hidden);

    /* Explicit memory structure lifecycle reclamation passes */
    if (xmlfile != NULL)
        g_object_unref(xmlfile);

    g_slist_free(MenuFiles);
    if (free_languages)
        g_strfreev(languages);

    g_free(ifile);
    g_free(ofile);
    g_free(lang);

    return rc;
}