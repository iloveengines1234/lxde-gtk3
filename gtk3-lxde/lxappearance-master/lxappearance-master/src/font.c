//      font.c
//
//      Copyright 2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      the Free Software Foundation; either version 3 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#include "lxappearance.h"
#include "font.h"
#include <glib/gi18n.h>

static const char* font_hinting_style[] = {
    "hintnone",
    "hintslight",
    "hintmedium",
    "hintfull"
};

static const char* font_rgba[] = {
    "none",
    "rgb",
    "bgr",
    "vrgb",
    "vbgr"
};

static void on_hinting_style_changed(GtkComboBox* combo, gpointer user_data)
{
    gint pos_combo = gtk_combo_box_get_active(combo);
    if (pos_combo >= 0 && pos_combo < (gint)G_N_ELEMENTS(font_hinting_style))
    {
        app.hinting_style = font_hinting_style[pos_combo];
        lxappearance_changed();
    }
}

static void on_font_rgba_changed(GtkComboBox* combo, gpointer user_data)
{
    gint pos_combo = gtk_combo_box_get_active(combo);
    if (pos_combo >= 0 && pos_combo < (gint)G_N_ELEMENTS(font_rgba))
    {
        app.font_rgba = font_rgba[pos_combo];
        lxappearance_changed();
    }
}

/* Dynamically evaluates array bounds to avoid memory reading vulnerabilities */
static int convert_list_to_pos(const char* font, const char* list[], int max_elements)
{
    int pos = -1, i;

    for (i = 0; i < max_elements; ++i)
    {
        if (g_strcmp0(list[i], font) == 0)
        {
            pos = i;
            break;
        }
    }

    return pos;
}

void font_init(GtkBuilder* b)
{
    int idx;
    
    app.enable_antialising_check = GTK_WIDGET(gtk_builder_get_object(b, "enable_antialising"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.enable_antialising_check), app.enable_antialising);
    g_signal_connect(app.enable_antialising_check, "toggled", G_CALLBACK(on_check_button_toggled), &app.enable_antialising);

    app.enable_hinting_check = GTK_WIDGET(gtk_builder_get_object(b, "enable_hinting"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app.enable_hinting_check), app.enable_hinting);
    g_signal_connect(app.enable_hinting_check, "toggled", G_CALLBACK(on_check_button_toggled), &app.enable_hinting);

    app.hinting_style_combo = GTK_WIDGET(gtk_builder_get_object(b, "hinting_style"));
    idx = convert_list_to_pos(app.hinting_style, font_hinting_style, G_N_ELEMENTS(font_hinting_style));
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.hinting_style_combo), idx);
    g_signal_connect(app.hinting_style_combo, "changed", G_CALLBACK(on_hinting_style_changed), NULL);

    app.font_rgba_combo = GTK_WIDGET(gtk_builder_get_object(b, "font_rgba"));
    idx = convert_list_to_pos(app.font_rgba, font_rgba, G_N_ELEMENTS(font_rgba));
    gtk_combo_box_set_active(GTK_COMBO_BOX(app.font_rgba_combo), idx);
    g_signal_connect(app.font_rgba_combo, "changed", G_CALLBACK(on_font_rgba_changed), NULL);
}