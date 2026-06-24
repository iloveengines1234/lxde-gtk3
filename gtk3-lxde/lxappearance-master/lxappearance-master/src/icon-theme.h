/*
 * icon-theme.h
 *
 * Copyright 2010 PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * the Free Software Foundation; either version 3 of the License.
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

#ifndef __ICON_THEME_H__
#define __ICON_THEME_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct
{
    char* name;
    char* disp_name;
    char* comment;
    const char* base_dir;
    gboolean has_icon : 1;
    gboolean has_cursor : 1;
    gboolean is_removable : 1;
} IconTheme;

/* Initialize icon theme components and wire UI signals */
void icon_theme_init(GtkBuilder* b);

/* Load icon theme data metadata records out of the designated target files */
void load_icon_themes_from_dir(const char* base_dir, const char* theme_dir, GKeyFile* kf);

/* Sorting and lookup comparison helper functions */
gint icon_theme_cmp_name(IconTheme* t, const char* name);
gint icon_theme_cmp_disp_name(IconTheme* t1, IconTheme* t2);

G_END_DECLS

#endif /* __ICON_THEME_H__ */