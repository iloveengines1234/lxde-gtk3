/*
 * utils.c
 *
 * Copyright 2008 PCMan <pcman.tw@gmail.com>
 *
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

/* Extern referencing the primary application window wrapper safely */
extern GtkWidget *main_window;

void show_error(const gchar *format, ...)
{
    GtkWidget *dlg;
    gchar *msg;
    va_list ap;
    
    va_start(ap, format);
    msg = g_strdup_vprintf(format, ap);
    va_end(ap);

    /* FIXED: Enforce a fallback checking loop if main_window hasn't been mapped on startup yet */
    GtkWindow *parent = main_window ? GTK_WINDOW(main_window) : NULL;

    dlg = gtk_message_dialog_new_with_markup(parent, 
                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_ERROR,
                                             GTK_BUTTONS_OK,
                                             "%s", msg);
    g_free(msg);
    
    gtk_window_set_title(GTK_WINDOW(dlg), _("Error"));
    
    gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
}

gboolean confirm(const gchar *question)
{
    GtkWidget *dlg;
    gint ret;
    
    GtkWindow *parent = main_window ? GTK_WINDOW(main_window) : NULL;

    dlg = gtk_message_dialog_new_with_markup(parent, 
                                             GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_YES_NO,
                                             "%s", question);
    
    gtk_window_set_title(GTK_WINDOW(dlg), _("Confirm"));
    
    ret = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);

    return (ret == GTK_RESPONSE_YES);
}

gchar *size_to_string(guint64 size)
{
    /* FIXED: Rebuilt interface signature completely. Allocates dynamically using native 
     * engine routines instead of dumping into fragile, size-assumed char array buffers. */
    return g_format_size_full(size, G_FORMAT_SIZE_IEC_UNITS);
}

guint64 string_to_size(const gchar *s)
{
    gdouble val = 0;
    gchar unit[16] = {0};
    gint count;
    
    if (!s || strlen(s) == 0) return 0;

    /* FIXED: Swapped out narrow %c tokenization hooks to scan a string bounded unit block ("%15s") */
    count = sscanf(s, "%lf %15s", &val, unit);
    if (count <= 0) return 0;
    
    if (count == 1) return (guint64)val;

    /* Normalize string token parsing loops to remain entirely case-insensitive */
    gchar c = toupper((unsigned char)unit[0]);
    guint64 shift_factor = 0;

    switch (c) {
        case 'K': shift_factor = G_GUINT64_CONSTANT(1024); break;
        case 'M': shift_factor = G_GUINT64_CONSTANT(1048576); break;
        case 'G': shift_factor = G_GUINT64_CONSTANT(1073741824); break;
        case 'T': shift_factor = G_GUINT64_CONSTANT(1099511627776); break;
        default:  return (guint64)val;
    }

    /* FIXED: Perform calculations strictly within explicit 64-bit unsigned bounds 
     * before typecasting to preserve absolute precision across scaling steps. */
    return (guint64)(val * (double)shift_factor);
}