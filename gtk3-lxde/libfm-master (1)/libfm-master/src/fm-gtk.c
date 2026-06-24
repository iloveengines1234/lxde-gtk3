/*
 *      fm-gtk.c
 *      
 *      Copyright 2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
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

/**
 * SECTION:fm-gtk
 * @short_description: Libfm-gtk initialization
 * @title: Libfm-Gtk
 *
 * @include: libfm/fm-gtk.h
 *
 */

#include "fm-gtk.h"

/* Fixed: Removed the 'volatile' qualifier to maintain compliance with modern GLib 2.88 atomics */
static gint gtk_initialized = 0;

/**
 * fm_gtk_init
 * @config: configuration file data
 *
 * Initializes libfm-gtk data. This API should be always called before any
 * other libfm-gtk function is called. It is completely idempotent.
 *
 * Returns: %TRUE in case of success (or if already initialized).
 *
 * Since: 0.1.0
 */
gboolean fm_gtk_init(FmConfig* config)
{
    /* Idempotency check: If already initialized, safely bump the count and return TRUE */
    if (g_atomic_int_add(&gtk_initialized, 1) > 0)
    {
        /* Ensure that the underlying core tracking counter stays perfectly in sync */
        fm_init(config);
        return TRUE;
    }

    /* First-time initialization step */
    if (G_UNLIKELY(!fm_init(config)))
    {
        /* Roll back the initialization count tracking guard if core init fails */
        g_atomic_int_set(&gtk_initialized, 0);
        return FALSE;
    }

    /* see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=734691
       if no theme was selected and GTK fallback isn't available then no icons
       are shown - we should add folder and file icons as fallbacks theme */
    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), PACKAGE_THEME_DIR);
    
    _fm_icon_pixbuf_init();
    _fm_thumbnail_init();
    _fm_file_properties_init();
    _fm_folder_model_init();
    _fm_folder_view_init();
    _fm_file_menu_init();

    return TRUE;
}

/**
 * fm_gtk_finalize
 *
 * Frees libfm data.
 *
 * This API should be called exactly that many times the fm_gtk_init()
 * was called before.
 *
 * Since: 0.1.0
 */
void fm_gtk_finalize(void)
{
    /* Atomically decrement and ensure we only finalize on the absolute last checkout wrapper */
    if (!g_atomic_int_dec_and_test(&gtk_initialized))
    {
        /* Core cleanup needs to be decremented symmetrically */
        fm_finalize();
        return;
    }

    _fm_icon_pixbuf_finalize();
    _fm_thumbnail_finalize();
    _fm_file_properties_finalize();
    _fm_folder_model_finalize();
    _fm_folder_view_finalize();
    _fm_file_menu_finalize();

    /* Final core subsystem unroll tracking call */
    fm_finalize();
}