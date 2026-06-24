/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   appearance.c for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens
   Copyright (c) 2003        Tim Riley
   Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "main.h"
#include "tree.h"
#include "preview_update.h"
#include "appearance.h"
#include <ctype.h> /* toupper */
#include <pango/pango-attributes.h>

static gboolean mapping = FALSE;

static RrFont *read_font(GtkFontButton *w, const gchar *place);
static RrFont *write_font(GtkFontButton *w, const gchar *place);

/* Forward Declarations */
void on_title_layout_changed(GtkEntry *w, gpointer data);
void on_font_active_font_set(GtkFontButton *w, gpointer data);
void on_font_inactive_font_set(GtkFontButton *w, gpointer data);
void on_font_menu_header_font_set(GtkFontButton *w, gpointer data);
void on_font_menu_item_font_set(GtkFontButton *w, gpointer data);
void on_font_active_display_font_set(GtkFontButton *w, gpointer data);
void on_font_inactive_display_font_set(GtkFontButton *w, gpointer data);

void appearance_setup_tab(void)
{
    GtkWidget *w;
    gchar *layout;
    RrFont *f;

    mapping = TRUE;

    w = get_widget("title_layout");
    layout = tree_get_string("theme/titleLayout", "NLIMC");
    gtk_entry_set_text(GTK_ENTRY(w), layout);
    preview_update_set_title_layout(layout);
    g_free(layout);

    w = get_widget("font_active");
    f = read_font(GTK_FONT_BUTTON(w), "ActiveWindow");
    preview_update_set_active_font(f);

    w = get_widget("font_inactive");
    f = read_font(GTK_FONT_BUTTON(w), "InactiveWindow");
    preview_update_set_inactive_font(f);

    w = get_widget("font_menu_header");
    f = read_font(GTK_FONT_BUTTON(w), "MenuHeader");
    preview_update_set_menu_header_font(f);

    w = get_widget("font_menu_item");
    f = read_font(GTK_FONT_BUTTON(w), "MenuItem");
    preview_update_set_menu_item_font(f);

    w = get_widget("font_active_display");
    if (!(f = read_font(GTK_FONT_BUTTON(w), "ActiveOnScreenDisplay"))) {
        f = read_font(GTK_FONT_BUTTON(w), "OnScreenDisplay");
        tree_delete_node("theme/font:place=OnScreenDisplay");
    }
    preview_update_set_osd_active_font(f);

    w = get_widget("font_inactive_display");
    f = read_font(GTK_FONT_BUTTON(w), "InactiveOnScreenDisplay");
    preview_update_set_osd_inactive_font(f);

    mapping = FALSE;
}

void on_title_layout_changed(GtkEntry *w, gpointer data)
{
    gchar *layout;
    gchar *it, *it2;
    gboolean n, d, s, l, i, m, c;

    if (mapping) return;

    layout = g_strdup(gtk_entry_get_text(w));
    if (!layout) return;

    n = d = s = l = i = m = c = FALSE;

    for (it = layout; *it; ++it) {
        gboolean *b = NULL;

        switch (*it) {
        case 'N': case 'n': b = &n; break;
        case 'D': case 'd': b = &d; break;
        case 'S': case 's': b = &s; break;
        case 'L': case 'l': b = &l; break;
        case 'I': case 'i': b = &i; break;
        case 'M': case 'm': b = &m; break;
        case 'C': case 'c': b = &c; break;
        default:  b = NULL; break;
        }

        if (!b || *b) {
            /* Drop duplicate/invalid tokens dynamically without shifting off limits */
            for (it2 = it; *it2; ++it2) {
                *it2 = *(it2 + 1);
            }
            if (*it == '\0') {
                break;
            }
            --it; /* Check replacement token at current cursor */
        } else {
            *it = (gchar)toupper((guchar)*it);
            *b = TRUE;
        }
    }

    gtk_entry_set_text(w, layout);
    tree_set_string("theme/titleLayout", layout);
    preview_update_set_title_layout(layout);

    g_free(layout);
}

void on_font_active_font_set(GtkFontButton *w, gpointer data)
{
    if (mapping) return;
    preview_update_set_active_font(write_font(w, "ActiveWindow"));
}

void on_font_inactive_font_set(GtkFontButton *w, gpointer data)
{
    if (mapping) return;
    preview_update_set_inactive_font(write_font(w, "InactiveWindow"));
}

void on_font_menu_header_font_set(GtkFontButton *w, gpointer data)
{
    if (mapping) return;
    preview_update_set_menu_header_font(write_font(w, "MenuHeader"));
}

void on_font_menu_item_font_set(GtkFontButton *w, gpointer data)
{
    if (mapping) return;
    preview_update_set_menu_item_font(write_font(w, "MenuItem"));
}

void on_font_active_display_font_set(GtkFontButton *w, gpointer data)
{
    if (mapping) return;
    /* Streamlined implementation directly using modern API signatures */
    preview_update_set_osd_active_font(write_font(w, "ActiveOnScreenDisplay"));
}

void on_font_inactive_display_font_set(GtkFontButton *w, gpointer data)
{
    if (mapping) return;
    preview_update_set_osd_inactive_font(write_font(w, "InactiveOnScreenDisplay"));
}

static RrFont *read_font(GtkFontButton *w, const gchar *place)
{
    RrFont *font;
    gchar *fontstring, *node;
    gchar *name, **names;
    gchar *size;
    gchar *weight;
    gchar *slant;

    RrFontWeight rr_weight = RR_FONTWEIGHT_NORMAL;
    RrFontSlant rr_slant = RR_FONTSLANT_NORMAL;

    mapping = TRUE;

    node = g_strdup_printf("theme/font:place=%s/name", place);
    name = tree_get_string(node, "Sans");
    g_free(node);

    node = g_strdup_printf("theme/font:place=%s/size", place);
    size = tree_get_string(node, "8");
    g_free(node);

    node = g_strdup_printf("theme/font:place=%s/weight", place);
    weight = tree_get_string(node, "");
    g_free(node);

    node = g_strdup_printf("theme/font:place=%s/slant", place);
    slant = tree_get_string(node, "");
    g_free(node);

    /* Tokenize first engine instance from layout font CSV maps safely */
    names = g_strsplit(name, ",", 0);
    g_free(name);
    name = g_strdup(names[0] ? names[0] : "Sans");
    g_strfreev(names);

    if (!g_ascii_strcasecmp(weight, "normal")) {
        g_free(weight); weight = g_strdup("");
    }
    if (!g_ascii_strcasecmp(slant, "normal")) {
        g_free(slant); slant = g_strdup("");
    }

    fontstring = g_strdup_printf("%s %s %s %s", name, weight, slant, size);
    
    /* Using native GTK 3 Font Chooser APIs natively */
    gtk_font_chooser_set_font(GTK_FONT_CHOOSER(w), fontstring);

    if (!g_ascii_strcasecmp(weight, "Bold")) rr_weight = RR_FONTWEIGHT_BOLD;
    if (!g_ascii_strcasecmp(slant, "Italic")) rr_slant = RR_FONTSLANT_ITALIC;
    if (!g_ascii_strcasecmp(slant, "Oblique")) rr_slant = RR_FONTSLANT_OBLIQUE;

    font = RrFontOpen(rrinst, name, atoi(size), rr_weight, rr_slant);
    
    g_free(fontstring);
    g_free(slant);
    g_free(weight);
    g_free(size);
    g_free(name);

    mapping = FALSE;

    return font;
}

static RrFont *write_font(GtkFontButton *w, const gchar *place)
{
    gchar *node;
    const gchar *font_family;
    gint font_size;
    gchar *weight_str = "Normal";
    gchar *slant_str = "Normal";

    RrFontWeight weight = RR_FONTWEIGHT_NORMAL;
    RrFontSlant slant = RR_FONTSLANT_NORMAL;
    RrFont *return_font = NULL;

    if (mapping) return NULL;

    /* Standardized GTK 3 execution block */
    gchar *font_desc_str = gtk_font_chooser_get_font(GTK_FONT_CHOOSER(w));
    PangoFontDescription *desc = pango_font_description_from_string(font_desc_str);
    g_free(font_desc_str);

    if (!desc) return NULL;

    font_family = pango_font_description_get_family(desc);
    if (!font_family) font_family = "Sans";

    font_size = pango_font_description_get_size(desc);
    if (!pango_font_description_get_size_is_absolute(desc)) {
        font_size = font_size / PANGO_SCALE;
    }
    if (font_size <= 0) font_size = 8;

    gchar *size_str = g_strdup_printf("%d", font_size);

    /* Match Weights */
    PangoWeight p_weight = pango_font_description_get_weight(desc);
    if (p_weight >= PANGO_WEIGHT_BOLD) {
        weight_str = "Bold";
        weight = RR_FONTWEIGHT_BOLD;
    }

    /* Match Slants */
    PangoStyle p_style = pango_font_description_get_style(desc);
    if (p_style == PANGO_STYLE_ITALIC) {
        slant_str = "Italic";
        slant = RR_FONTSLANT_ITALIC;
    } else if (p_style == PANGO_STYLE_OBLIQUE) {
        slant_str = "Oblique";
        slant = RR_FONTSLANT_OBLIQUE;
    }

    /* Write parsed configurations to the backend ObConf tree */
    node = g_strdup_printf("theme/font:place=%s/name", place);
    tree_set_string(node, font_family);
    g_free(node);

    node = g_strdup_printf("theme/font:place=%s/size", place);
    tree_set_string(node, size_str);
    g_free(node);

    node = g_strdup_printf("theme/font:place=%s/weight", place);
    tree_set_string(node, weight_str);
    g_free(node);

    node = g_strdup_printf("theme/font:place=%s/slant", place);
    tree_set_string(node, slant_str);
    g_free(node);

    /* Allocate Openbox Render Font instance cleanly */
    return_font = RrFontOpen(rrinst, font_family, font_size, weight, slant);

    /* Memory Cleanups */
    g_free(size_str);
    pango_font_description_free(desc);

    return return_font;
}