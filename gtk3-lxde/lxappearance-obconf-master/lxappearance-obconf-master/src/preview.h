/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   preview.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2007        Javeed Shaikh
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_PREVIEW_H
#define OBCONF_PREVIEW_H

#include <glib.h>
#include <gdk/gdk.h>
#include <obrender/font.h>

G_BEGIN_DECLS

/**
 * preview_theme:
 * @name: Name of the Openbox theme to load and parse.
 * @titlelayout: Button/Label placement layout configuration string (e.g., "NLIMC").
 * @active_window_font: Font structure utilized for focused window titles.
 * @inactive_window_font: Font structure utilized for unfocused window titles.
 * @menu_title_font: Font structure utilized for menu headers.
 * @menu_item_font: Font structure utilized for individual menu entries.
 * @osd_active_font: Font structure utilized for active on-screen displays.
 * @osd_inactive_font: Font structure utilized for inactive on-screen displays.
 *
 * Generates a unified preview image containing render composites of both the
 * window decoration frames and standard menu components for UI engine rendering.
 *
 * Returns: (transfer full): A newly allocated #GdkPixbuf containing the visual theme,
 * or %NULL if theme initialization failed.
 */
GdkPixbuf *preview_theme(const gchar *name, const gchar *titlelayout,
                         RrFont *active_window_font,
                         RrFont *inactive_window_font,
                         RrFont *menu_title_font,
                         RrFont *menu_item_font,
                         RrFont *osd_active_font,
                         RrFont *osd_inactive_font);

G_END_DECLS

#endif /* OBCONF_PREVIEW_H */