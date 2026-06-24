/*
 *      fm-app-lookup.c
 *
 *      Copyright 2010 PCMan <pcman.tw@gmail.com>
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

#include "fm-app-lookup.h"
#include <gio/gio.h>

/**
 * fm_app_lookup_get_default_for_uri_scheme:
 * @scheme: The URI scheme (e.g., "http", "mailto")
 * * Returns the default application assigned to handle the given URI scheme.
 * * Returns: (transfer full): a #GAppInfo, or NULL if none could be found.
 */
GAppInfo *fm_app_lookup_get_default_for_uri_scheme(const char *scheme)
{
    GAppInfo *app = NULL;
    
    if (g_strcmp0(scheme, "mailto") == 0) {
        /* Fallback or specific logic for mail clients if needed, 
         * otherwise standard x-scheme-handler catches it. */
    }

    /* Standard way to look up scheme handlers in GLib 2.28+ / GTK 3 */
    char *mime_type = g_strconcat("x-scheme-handler/", scheme, NULL);
    app = g_app_info_get_default_for_type(mime_type, FALSE);
    g_free(mime_type);
    
    return app;
}