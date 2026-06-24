/*
 * archive.c for ObConf, the configuration tool for Openbox
 * Copyright (c) 2003-2007   Dana Jansens
 * Copyright (c) 2003        Tim Riley
 * Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 * Copyright (C) 2026        LXDE Maintainers Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "theme.h"
#include "main.h"
#include "archive.h"
#include <glib/gi18n-lib.h>

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

/* Modernized non-blocking format-safe replacement for the deprecated macro */
static void show_message_dialog(GtkMessageType type, const gchar *format, ...)
{
    GtkWidget *msgw;
    gchar *message_text;
    va_list args;

    va_start(args, format);
    message_text = g_strdup_vprintf(format, args);
    va_end(args);

    msgw = gtk_message_dialog_new(mainwin ? GTK_WINDOW(mainwin) : NULL,
                                  GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
                                  type,
                                  GTK_BUTTONS_OK,
                                  "%s", message_text);

    /* Enforce window hierarchy layout management rules */
    if (mainwin) {
        gtk_window_set_transient_for(GTK_WINDOW(msgw), GTK_WINDOW(mainwin));
    }

    gtk_window_set_title(GTK_WINDOW(msgw), (type == GTK_MESSAGE_ERROR) ? _("Error") : _("Information"));
    
    /* Connect to automatically destroy on user interaction without using deprecated gtk_dialog_run */
    g_signal_connect(msgw, "response", G_CALLBACK(gtk_widget_destroy), NULL);
    
    /* Using native gtk_widget_show on GTK 3 to maintain accurate internal structural drawing */
    gtk_widget_show(msgw);

    g_free(message_text);
}

static gchar *get_theme_dir(void);
static gchar *name_from_dir(const gchar *dir);
static gchar *install_theme_to(const gchar *file, const gchar *to);
static gboolean create_theme_archive(const gchar *dir, const gchar *name, const gchar *to);

gchar* archive_install(const gchar *path)
{
    gchar *dest;
    gchar *name;

    if (!(dest = get_theme_dir()))
        return NULL;

    if ((name = install_theme_to(path, dest))) {
        show_message_dialog(GTK_MESSAGE_INFO, _("\"%s\" was installed to %s"), name, dest);
    }

    g_free(dest);
    return name;
}

void archive_create(const gchar *path)
{
    gchar *name;
    gchar *dest;
    gchar *current_dir;

    if (!(name = name_from_dir(path)))
        return;

    current_dir = g_get_current_dir();
    if (current_dir) {
        gchar *file = g_strdup_printf("%s.obt", name);
        dest = g_build_path(G_DIR_SEPARATOR_S, current_dir, file, NULL);
        g_free(file);
        g_free(current_dir);
    } else {
        g_free(name);
        return;
    }

    if (create_theme_archive(path, name, dest)) {
        show_message_dialog(GTK_MESSAGE_INFO, _("\"%s\" was successfully created"), dest);
    }

    g_free(dest);
    g_free(name);
}

static gboolean create_theme_archive(const gchar *dir, const gchar *name, const gchar *to)
{
    gchar *glob;
    gchar *errtxt = NULL;
    gchar *parentdir;
    gint exitcode = -1; 
    GError *e = NULL;
    gboolean run_success;

    glob = g_strdup_printf("%s/openbox-3/", name);
    parentdir = g_build_path(G_DIR_SEPARATOR_S, dir, "..", NULL);

    /* Safe auto-terminated list allocation without arbitrary magic integer constraints */
    gchar *argv[] = {
        "tar",
        "-c",
        "-z",
        "-f",
        (gchar *)to,
        "-C",
        parentdir,
        glob,
        NULL
    };

    run_success = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                               NULL, &errtxt, &exitcode, &e);

    if (run_success) {
        if (exitcode != EXIT_SUCCESS) {
            show_message_dialog(GTK_MESSAGE_ERROR,
                                _("Unable to create the theme archive \"%s\".\nThe following errors were reported:\n%s"),
                                to, errtxt ? errtxt : "");
        }
    } else {
        show_message_dialog(GTK_MESSAGE_ERROR, _("Unable to run the \"tar\" command: %s"),
                            e ? e->message : _("Unknown error"));
    }

    if (e) g_error_free(e);
    g_free(errtxt);
    g_free(parentdir);
    g_free(glob);

    return (run_success && exitcode == EXIT_SUCCESS);
}

static gchar *get_theme_dir(void)
{
    gchar *dir;
    gint r;

    dir = g_build_path(G_DIR_SEPARATOR_S, g_get_home_dir(), ".themes", NULL);
    r = g_mkdir_with_parents(dir, 0755); 
    
    if (r == -1 && errno != EEXIST) {
        show_message_dialog(GTK_MESSAGE_ERROR,
                            _("Unable to create directory \"%s\": %s"),
                            dir, g_strerror(errno));
        g_free(dir);
        dir = NULL;
    }

    return dir;
}

static gchar* name_from_dir(const gchar *dir)
{
    gchar *rc;
    GStatBuf st;
    gboolean r;

    rc = g_build_path(G_DIR_SEPARATOR_S, dir, "openbox-3", "themerc", NULL);
    r = (g_stat(rc, &st) == 0 && S_ISREG(st.st_mode));
    g_free(rc);

    if (!r) {
        show_message_dialog(GTK_MESSAGE_ERROR,
                            _("\"%s\" does not appear to be a valid Openbox theme directory"),
                            dir);
        return NULL;
    }
    return g_path_get_basename(dir);
}

static gchar* install_theme_to(const gchar *file, const gchar *to)
{
    gchar *errtxt = NULL, *outtxt = NULL;
    gint exitcode = -1;
    GError *e = NULL;
    gchar *name = NULL;
    gboolean run_success;

    gchar *argv[] = {
        "tar",
        "-x",
        "-v",
        "-z",
        "--wildcards",
        "-f",
        (gchar *)file,
        "-C",
        (gchar *)to,
        "*/openbox-3/",
        NULL
    };

    run_success = g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                               &outtxt, &errtxt, &exitcode, &e);

    if (!run_success) {
        show_message_dialog(GTK_MESSAGE_ERROR, _("Unable to run the \"tar\" command: %s"),
                            e ? e->message : _("Unknown error"));
    }
    if (e) g_error_free(e);

    if (run_success && exitcode != EXIT_SUCCESS) {
        show_message_dialog(GTK_MESSAGE_ERROR,
                            _("Unable to extract the file \"%s\".\nPlease ensure that \"%s\" is writable and that the file is a valid Openbox theme archive.\nThe following errors were reported:\n%s"),
                            file, to, errtxt ? errtxt : "");
    }

    if (run_success && exitcode == EXIT_SUCCESS && outtxt) {
        gchar **lines = g_strsplit(outtxt, "\n", 0);
        gchar **it;
        for (it = lines; *it; ++it) {
            gchar *l = *it;
            gchar *match = strstr(l, "/openbox-3/");

            if (match) {
                *match = '\0'; /* safely truncate the match path string boundaries */
                name = g_strdup(*it);
                break;
            }
        }
        g_strfreev(lines);
    }

    g_free(outtxt);
    g_free(errtxt);
    return name;
}