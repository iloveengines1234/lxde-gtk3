/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   preview_update.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2008   Dana Jansens
   Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_PREVIEW_UPDATE_H
#define OBCONF_PREVIEW_UPDATE_H

#include <gtk/gtk.h>
#include <obrender/font.h>

G_BEGIN_DECLS

/**
 * preview_update_all:
 *
 * Re-renders the theme preview pane using the active style data configuration properties.
 */
void preview_update_all(void);

/**
 * preview_update_set_tree_view:
 * @tr: The target #GtkTreeView widget showing theme selections.
 * @ls: The backing #GtkListStore data framework model.
 *
 * Configures the primary tree model layout definitions for preview tracking.
 */
void preview_update_set_tree_view(GtkTreeView *tr, GtkListStore *ls);

/* Core Font Engine State Setters */
void preview_update_set_active_font(RrFont *f);
void preview_update_set_inactive_font(RrFont *f);
void preview_update_set_menu_header_font(RrFont *f);
void preview_update_set_menu_item_font(RrFont *f);
void preview_update_set_osd_active_font(RrFont *f);
void preview_update_set_osd_inactive_font(RrFont *f);

/**
 * preview_update_set_title_layout:
 * @layout: Openbox-formatted window button string sequence.
 *
 * Dispatches updates to rearrange titlebar element configurations.
 */
void preview_update_set_title_layout(const gchar *layout);

G_END_DECLS

#endif /* OBCONF_PREVIEW_UPDATE_H */