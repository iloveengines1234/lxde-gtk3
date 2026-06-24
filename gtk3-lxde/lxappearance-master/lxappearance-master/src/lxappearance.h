/*
 * lxappearance.h
 *
 * Copyright 2010 PCMan <pcman.tw@gmail.com>
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

#ifndef __LXAPPEARANCE_H__
#define __LXAPPEARANCE_H__

#include <gtk/gtk.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#endif

#define LXAPPEARANCE_ABI_VERSION    1

typedef struct _LXAppearance    LXAppearance;
struct _LXAppearance
{
    guint32 abi_version;
    GtkWidget* dlg;
    GtkWidget* notebook;

    /* gtk theme */
    GtkWidget* widget_theme_view;
    GtkListStore* widget_theme_store;
    GtkWidget* default_font_btn;

    /* color scheme */
    GtkWidget* color_table;
    GtkWidget* custom_colors;
    GtkWidget* no_custom_colors;
    GHashTable* color_scheme_hash;          /* the custom color scheme set by the user */
    GHashTable* default_color_scheme_hash;  /* default colors of current gtk theme */
    gboolean color_scheme_supported;        /* if color scheme is supported by current gtk theme */
    
    /* color buttons in color scheme page. */
    GtkWidget* color_btns[8];               /* FIXME: this value might be changed in the future */

    /* icon theme */
    GtkWidget* icon_theme_view;
    GtkListStore* icon_theme_store;
    GtkWidget* icon_theme_remove_btn;

    /* cursor theme */
    GtkWidget* cursor_theme_view;
    GtkWidget* cursor_demo_view;
    GtkListStore* cursor_theme_store;
    GtkWidget* cursor_size_range;
    GtkWidget* cursor_theme_remove_btn;

    GSList* icon_themes; /* a list of IconTheme struct representing all icon and cursor themes */

    /* toolbar style and icon size */
    GtkWidget* tb_style_combo;
    GtkWidget* tb_icon_size_combo;

    GtkWidget* button_images_check;
    GtkWidget* menu_images_check;

    /* font rendering */
    GtkWidget* font_rgba_combo;
    GtkWidget* hinting_style_combo;

    GtkWidget* enable_antialising_check;
    GtkWidget* enable_hinting_check;

    /* the page for window manager plugins */
    GtkWidget* wm_page;

    char* widget_theme;
    char* default_font;
    char* icon_theme;
    char* cursor_theme;
    int cursor_theme_size;
    char* color_scheme;
    int toolbar_style;
    int toolbar_icon_size;
    const char* hinting_style;
    const char* font_rgba;

    gboolean button_images;
    gboolean menu_images;

    /* Sound Properties (Native components under GTK3 environments) */
    GtkWidget* event_sound_check;
    GtkWidget* input_feedback_check;
    gboolean enable_event_sound;
    gboolean enable_input_feedback;

    gboolean enable_antialising;
    gboolean enable_hinting;

    gboolean changed;
    gboolean use_lxsession;

    char *modules;
    GtkWidget *enable_accessibility_button;
};

extern LXAppearance app;

#ifdef GDK_WINDOWING_X11
extern Atom lxsession_atom;
#endif

/* Set state tracking variable assertions when UI features are manipulated */
void lxappearance_changed(void);

/* Shared cross-module callback handler for generic checkbox configurations */
void on_check_button_toggled(GtkToggleButton* btn, gpointer user_data);

#endif /* __LXAPPEARANCE_H__ */