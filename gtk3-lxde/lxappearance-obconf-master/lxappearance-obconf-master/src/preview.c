/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   preview.c for ObConf, the configuration tool for Openbox
   Copyright (c) 2007        Javeed Shaikh
   Copyright (c) 2007        Dana Jansens
   Copyright (C) 2023        Ingo Brückl
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "theme.h"
#include "main.h"
#include "tree.h"

#include <string.h>
#include <obrender/theme.h>

#if GTK_CHECK_VERSION(3, 0, 0)
#include <cairo-xlib.h>
#include <gdk/gdkx.h>
#endif

#define PADDING 2 /* openbox match boundary constraints */

/* Forwarded Prototypes */
GdkPixbuf *preview_theme(const gchar *name, const gchar *titlelayout,
                         RrFont *active_window_font,
                         RrFont *inactive_window_font,
                         RrFont *menu_title_font,
                         RrFont *menu_item_font,
                         RrFont *osd_active_font,
                         RrFont *osd_inactive_font);

static void theme_pixmap_paint(RrAppearance *a, gint w, gint h);
static guint32 rr_color_pixel(const RrColor *c);
static GdkPixbuf* preview_menu(RrTheme *theme);
static GdkPixbuf* preview_window(RrTheme *theme, const gchar *titlelayout, gboolean focus, gint width, gint height);
static gint theme_label_width(RrTheme *theme, gboolean active);
static gint theme_window_min_width(RrTheme *theme, const gchar *titlelayout);

static void theme_pixmap_paint(RrAppearance *a, gint w, gint h)
{
    Pixmap out = RrPaintPixmap(a, w, h);
    if (out) {
        XFreePixmap(RrDisplay(a->inst), out);
    }
}

static guint32 rr_color_pixel(const RrColor *c)
{
    return (guint32)((RrColorRed(c) << 24) + (RrColorGreen(c) << 16) +
                     (RrColorBlue(c) << 8) + 0xff);
}

#if GTK_CHECK_VERSION(3, 0, 0)
static GdkPixbuf *pixbuf_get_from_pixmap(GdkPixbuf *dest, Drawable src,
                                         int dest_x, int dest_y,
                                         int width, int height)
{
    Display *dpy;
    cairo_surface_t *surface;
    GdkPixbuf *pixbuf;

    dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    surface = cairo_xlib_surface_create(dpy, src, DefaultVisual(dpy, 0), width, height);
    pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
    if (pixbuf) {
        gdk_pixbuf_copy_area(pixbuf, 0, 0, width, height, dest, dest_x, dest_y);
        g_object_unref(pixbuf);
    }
    cairo_surface_destroy(surface);

    return dest;
}
#endif

static GdkPixbuf* preview_menu(RrTheme *theme)
{
    RrAppearance *title;
    RrAppearance *title_text;
    RrAppearance *menu;
    RrAppearance *background;
    RrAppearance *normal;
    RrAppearance *disabled;
    RrAppearance *selected;
    RrAppearance *bullet;

#if !GTK_CHECK_VERSION(3, 0, 0)
    GdkPixmap *pixmap;
#endif
    GdkPixbuf *pixbuf;

    gint width, height;
    gint x, y;
    gint title_h;
    gint tw, th;
    gint bw, bh;
    gint unused;

    menu = theme->a_menu;
    title = theme->a_menu_title;

    if (title->surface.grad == RR_SURFACE_PARENTREL)
        title->surface.parent = menu;

    title_text = theme->a_menu_text_title;
    title_text->surface.parent = title;
    title_text->texture[0].data.text.string = "Menu";

    normal = theme->a_menu_text_normal;
    normal->texture[0].data.text.string = "Normal";

    disabled = theme->a_menu_text_disabled;
    disabled->texture[0].data.text.string = "Disabled";

    selected = theme->a_menu_text_selected;
    selected->texture[0].data.text.string = "Selected";

    bullet = theme->a_menu_bullet_normal;

    RrMinSize(normal, &width, &th);
    width += th + PADDING; 
    width += 2 * theme->mbwidth + 2 * PADDING;

    RrMinSize(title, &tw, &title_h);

    bw = width - 2 * theme->mbwidth;
    title_h = theme->menu_title_height;

    RrMinSize(normal, &unused, &th);
    bh = th + 2 * PADDING;

    height = title_h + 3 * bh + 3 * theme->mbwidth;

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    if (!pixbuf) return NULL;
    
    gdk_pixbuf_fill(pixbuf, rr_color_pixel(theme->menu_border_color));

    x = y = theme->mbwidth;
    theme_pixmap_paint(title, bw, title_h);

    title_text->surface.parentx = 0;
    title_text->surface.parenty = 0;
    theme_pixmap_paint(title_text, bw, title_h);

    if (title_text->pixmap != None) {
#if GTK_CHECK_VERSION(3, 0, 0)
        pixbuf = pixbuf_get_from_pixmap(pixbuf, title_text->pixmap, x, y, bw, title_h);
#else
        pixmap = gdk_pixmap_foreign_new(title_text->pixmap);
        pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, bw, title_h);
        g_object_unref(pixmap);
#endif
    }
    
    y += theme->mbwidth + title_h;

    th = height - 3 * theme->mbwidth - title_h;
    theme_pixmap_paint(menu, bw, th);

#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, menu->pixmap, x, y, bw, th);
#else
    pixmap = gdk_pixmap_foreign_new(menu->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, bw, th);
    g_object_unref(pixmap);
#endif

    background = theme->a_menu_normal;
    background->surface.parent = menu;
    background->surface.parentx = 0;
    background->surface.parenty = 0;

    theme_pixmap_paint(background, bw, bh);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, background->pixmap, x, y, bw, bh);
#else
    pixmap = gdk_pixmap_foreign_new(background->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, bw, bh);
    g_object_unref(pixmap);
#endif

    normal->surface.parent = background;
    normal->surface.parentx = PADDING;
    normal->surface.parenty = PADDING;
    
    gint normal_x = x + PADDING;
    gint normal_y = y + PADDING;
    
    RrMinSize(normal, &tw, &th);
    theme_pixmap_paint(normal, tw, th);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, normal->pixmap, normal_x, normal_y, tw, th);
#else
    pixmap = gdk_pixmap_foreign_new(normal->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, normal_x, normal_y, tw, th);
    g_object_unref(pixmap);
#endif

    bullet->surface.parent = background;
    bullet->surface.parentx = bw - th;
    bullet->surface.parenty = PADDING;
    theme_pixmap_paint(bullet, th, th);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, bullet->pixmap, width - theme->mbwidth - th, normal_y, th, th);
#else
    pixmap = gdk_pixmap_foreign_new(bullet->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, width - theme->mbwidth - th, normal_y, th, th);
    g_object_unref(pixmap);
#endif
    y += th + 2 * PADDING;

    background->surface.parenty = bh;
    theme_pixmap_paint(background, bw, bh);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, background->pixmap, x, y, bw, bh);
#else
    pixmap = gdk_pixmap_foreign_new(background->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, bw, bh);
    g_object_unref(pixmap);
#endif

    RrMinSize(disabled, &tw, &th);
    disabled->surface.parent = background;
    disabled->surface.parentx = PADDING;
    disabled->surface.parenty = PADDING;
    theme_pixmap_paint(disabled, tw, th);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, disabled->pixmap, x + PADDING, y + PADDING, tw, th);
#else
    pixmap = gdk_pixmap_foreign_new(disabled->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x + PADDING, y + PADDING, tw, th);
    g_object_unref(pixmap);
#endif
    y += th + 2 * PADDING;

    background = theme->a_menu_selected;
    background->surface.parent = menu;
    background->surface.parentx = 2 * bh;

    theme_pixmap_paint(background, bw, bh);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, background->pixmap, x, y, bw, bh);
#else
    pixmap = gdk_pixmap_foreign_new(background->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, bw, bh);
    g_object_unref(pixmap);
#endif

    RrMinSize(selected, &tw, &th);
    selected->surface.parent = background;
    selected->surface.parentx = PADDING;
    selected->surface.parenty = PADDING;
    theme_pixmap_paint(selected, tw, th);
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, selected->pixmap, x + PADDING, y + PADDING, tw, th);
#else
    pixmap = gdk_pixmap_foreign_new(selected->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x + PADDING, y + PADDING, tw, th);
    g_object_unref(pixmap);
#endif

    return pixbuf;
}

static GdkPixbuf* preview_window(RrTheme *theme, const gchar *titlelayout,
                                 gboolean focus, gint width, gint height)
{
    RrAppearance *title;
    RrAppearance *handle;
    RrAppearance *a;
#if !GTK_CHECK_VERSION(3, 0, 0)
    GdkPixmap *pixmap;
#endif
    GdkPixbuf *pixbuf = NULL;
    GdkPixbuf *scratch;

    gint w, label_w, h, x, y;
    const gchar *layout;

    title = focus ? theme->a_focused_title : theme->a_unfocused_title;

    pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    if (!pixbuf) return NULL;

    gdk_pixbuf_fill(pixbuf,
                    rr_color_pixel(focus ?
                                   theme->frame_focused_border_color :
                                   theme->frame_unfocused_border_color));

    w = width - 2 * theme->fbwidth;
    h = theme->title_height;
    theme_pixmap_paint(title, w, h);

    x = y = theme->fbwidth;
#if GTK_CHECK_VERSION(3, 0, 0)
    pixbuf = pixbuf_get_from_pixmap(pixbuf, title->pixmap, x, y, w, h);
#else
    pixmap = gdk_pixmap_foreign_new(title->pixmap);
    pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, w, h);
    g_object_unref(pixmap);
#endif

    label_w = width - (theme->paddingx + theme->fbwidth + 1) * 2;

    for (layout = titlelayout; *layout; layout++) {
        switch (*layout) {
        case 'N':
            label_w -= theme->button_size + 2 + theme->paddingx + 1;
            break;
        case 'D':
        case 'S':
        case 'I':
        case 'M':
        case 'C':
            label_w -= theme->button_size + theme->paddingx + 1;
            break;
        default:
            break;
        }
    }

    x = theme->paddingx + theme->fbwidth + 1;
    y += theme->paddingy;
    for (layout = titlelayout; *layout; layout++) {
        if (*layout == 'N') {
            a = theme->a_icon;
            a->texture[0].type = RR_TEXTURE_RGBA;
            a->texture[0].data.rgba.width = 48;
            a->texture[0].data.rgba.height = 48;
            a->texture[0].data.rgba.alpha = 0xff;
            a->texture[0].data.rgba.data = theme->def_win_icon;

            a->surface.parent = title;
            a->surface.parentx = x - theme->fbwidth;
            a->surface.parenty = theme->paddingy;

            w = h = theme->button_size + 2;
            theme_pixmap_paint(a, w, h);
#if GTK_CHECK_VERSION(3, 0, 0)
            pixbuf = pixbuf_get_from_pixmap(pixbuf, a->pixmap, x, y, w, h);
#else
            pixmap = gdk_pixmap_foreign_new(a->pixmap);
            pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, w, h);
            g_object_unref(pixmap);
#endif
            x += theme->button_size + 2 + theme->paddingx + 1;
        } else if (*layout == 'L') {
            a = focus ? theme->a_focused_label : theme->a_unfocused_label;
            a->texture[0].data.text.string = focus ? "Active" : "Inactive";

            a->surface.parent = title;
            a->surface.parentx = x - theme->fbwidth;
            a->surface.parenty = theme->paddingy;
            w = label_w;
            h = theme->label_height;

            theme_pixmap_paint(a, w, h);
#if GTK_CHECK_VERSION(3, 0, 0)
            pixbuf = pixbuf_get_from_pixmap(pixbuf, a->pixmap, x, y, w, h);
#else
            pixmap = gdk_pixmap_foreign_new(a->pixmap);
            pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, w, h);
            g_object_unref(pixmap);
#endif
            x += w + theme->paddingx + 1;
        } else {
            switch (*layout) {
            case 'D':
                a = focus ?
#if RR_CHECK_VERSION(3, 5, 28)
                    theme->btn_desk->a_focused_unpressed : theme->btn_desk->a_unfocused_unpressed;
#else
                    theme->a_focused_unpressed_desk : theme->a_unfocused_unpressed_desk;
#endif
                break;
            case 'S':
                a = focus ?
#if RR_CHECK_VERSION(3, 5, 28)
                    theme->btn_shade->a_focused_unpressed : theme->btn_shade->a_unfocused_unpressed;
#else
                    theme->a_focused_unpressed_shade : theme->a_unfocused_unpressed_shade;
#endif
                break;
            case 'I':
                a = focus ?
#if RR_CHECK_VERSION(3, 5, 28)
                    theme->btn_iconify->a_focused_unpressed : theme->btn_iconify->a_unfocused_unpressed;
#else
                    theme->a_focused_unpressed_iconify : theme->a_unfocused_unpressed_iconify;
#endif
                break;
            case 'M':
                a = focus ?
#if RR_CHECK_VERSION(3, 5, 28)
                    theme->btn_max->a_focused_unpressed : theme->btn_max->a_unfocused_unpressed;
#else
                    theme->a_focused_unpressed_max : theme->a_unfocused_unpressed_max;
#endif
                break;
            case 'C':
                a = focus ?
#if RR_CHECK_VERSION(3, 5, 28)
                    theme->btn_close->a_focused_unpressed : theme->btn_close->a_unfocused_unpressed;
#else
                    theme->a_focused_unpressed_close : theme->a_unfocused_unpressed_close;
#endif
                break;
            default:
                continue;
            }

            a->surface.parent = title;
            a->surface.parentx = x - theme->fbwidth;
            a->surface.parenty = theme->paddingy + 1;

            w = theme->button_size;
            h = theme->button_size;
            theme_pixmap_paint(a, w, h);
#if GTK_CHECK_VERSION(3, 0, 0)
            pixbuf = pixbuf_get_from_pixmap(pixbuf, a->pixmap, x, y + 1, w, h);
#else
            pixmap = gdk_pixmap_foreign_new(a->pixmap);
            pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y + 1, w, h);
            g_object_unref(pixmap);
#endif
            x += theme->button_size + theme->paddingx + 1;
        }
    }

    if (theme->handle_height) {
        handle = focus ? theme->a_focused_handle : theme->a_unfocused_handle;
        x = 2 * theme->fbwidth + theme->grip_width;
        y = height - theme->fbwidth - theme->handle_height;
        w = width - 4 * theme->fbwidth - 2 * theme->grip_width;
        h = theme->handle_height;

        theme_pixmap_paint(handle, w, h);
#if GTK_CHECK_VERSION(3, 0, 0)
        pixbuf = pixbuf_get_from_pixmap(pixbuf, handle->pixmap, x, y, w, h);
#else
        pixmap = gdk_pixmap_foreign_new(handle->pixmap);
        pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, w, h);
        g_object_unref(pixmap);
#endif
        theme_pixmap_paint(handle, width, h);

        a = focus ? theme->a_focused_grip : theme->a_unfocused_grip;
        a->surface.parent = handle;

        x = theme->fbwidth;
        w = theme->grip_width;
        theme_pixmap_paint(a, w, h);
#if GTK_CHECK_VERSION(3, 0, 0)
        pixbuf = pixbuf_get_from_pixmap(pixbuf, a->pixmap, x, y, w, h);
#else
        pixmap = gdk_pixmap_foreign_new(a->pixmap);
        pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, w, h);
        g_object_unref(pixmap);
#endif
        x = width - theme->fbwidth - theme->grip_width;
#if GTK_CHECK_VERSION(3, 0, 0)
        pixbuf = pixbuf_get_from_pixmap(pixbuf, a->pixmap, x, y, w, h);
#else
        pixmap = gdk_pixmap_foreign_new(a->pixmap);
        pixbuf = gdk_pixbuf_get_from_drawable(pixbuf, pixmap, gdk_colormap_get_system(), 0, 0, x, y, w, h);
        g_object_unref(pixmap);
#endif
    }

    x = theme->fbwidth;
    y = theme->fbwidth + theme->title_height;
    w = width - 2 * theme->fbwidth;
    h = theme->fbwidth;

    if (h > 0) {
        scratch = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
        if (scratch) {
            gdk_pixbuf_fill(scratch, rr_color_pixel(focus ?
                                                    theme->title_separator_focused_color :
                                                    theme->title_separator_unfocused_color));
            gdk_pixbuf_copy_area(scratch, 0, 0, w, h, pixbuf, x, y);
            g_object_unref(scratch);
        }
    }

    x = theme->fbwidth;
    y = theme->title_height + 2 * theme->fbwidth;
    w = width - 2 * theme->fbwidth;
    h = height - theme->title_height - 3 * theme->fbwidth -
        (theme->handle_height ? (theme->fbwidth + theme->handle_height) : 0);

    scratch = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, w, h);
    if (scratch) {
        gdk_pixbuf_fill(scratch, rr_color_pixel(focus ?
                                                theme->cb_focused_color :
                                                theme->cb_unfocused_color));
        gdk_pixbuf_copy_area(scratch, 0, 0, w, h, pixbuf, x, y);

        gdk_pixbuf_fill(scratch, 0xffffffff);
        gdk_pixbuf_copy_area(scratch, 0, 0,
                             w - 2 * theme->cbwidthx,
                             h - 2 * theme->cbwidthy,
                             pixbuf,
                             x + theme->cbwidthx,
                             y + theme->cbwidthy);
        g_object_unref(scratch);
    }

    return pixbuf;
}

static gint theme_label_width(RrTheme *theme, gboolean active)
{
    RrAppearance *label;

    if (active) {
        label = theme->a_focused_label;
        label->texture[0].data.text.string = "Active";
    } else {
        label = theme->a_unfocused_label;
        label->texture[0].data.text.string = "Inactive";
    }

    return RrMinWidth(label);
}

static gint theme_window_min_width(RrTheme *theme, const gchar *titlelayout)
{
    gint numbuttons = strlen(titlelayout);
    gint w = 2 * theme->fbwidth + (numbuttons + 3) * (theme->paddingx + 1);

    if (g_strrstr(titlelayout, "L")) {
        numbuttons--;
        w += MAX(theme_label_width(theme, TRUE),
                 theme_label_width(theme, FALSE));
    }

    w += theme->button_size * numbuttons;
    return w;
}

GdkPixbuf *preview_theme(const gchar *name, const gchar *titlelayout,
                         RrFont *active_window_font,
                         RrFont *inactive_window_font,
                         RrFont *menu_title_font,
                         RrFont *menu_item_font,
                         RrFont *osd_active_font,
                         RrFont *osd_inactive_font)
{
    GdkPixbuf *preview;
    GdkPixbuf *menu;
    GdkPixbuf *window;

    gint window_w;
    gint menu_w;
    gint w, h;

    RrTheme *theme = RrThemeNew(rrinst, name, FALSE,
                                active_window_font, inactive_window_font,
                                menu_title_font, menu_item_font,
                                osd_active_font, osd_inactive_font);
    if (!theme)
        return NULL;

    menu = preview_menu(theme);
    window_w = theme_window_min_width(theme, titlelayout);

    menu_w = menu ? gdk_pixbuf_get_width(menu) : 0;
    h = menu ? gdk_pixbuf_get_height(menu) : 150; /* resilient fallback metric */
    w = MAX(window_w, menu_w) + 20;
  
    if (!window_w) window_w = menu_w;

    preview = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8,
                             w, h + 2 * (theme->title_height + 5) + 1);
    if (!preview) {
        if (menu) g_object_unref(menu);
        RrThemeFree(theme);
        return NULL;
    }
    gdk_pixbuf_fill(preview, 0);

    window = preview_window(theme, titlelayout, FALSE, window_w, h);
    if (window) {
        gdk_pixbuf_copy_area(window, 0, 0, window_w, h, preview, 20, 0);
        g_object_unref(window);
    }

    window = preview_window(theme, titlelayout, TRUE, window_w, h);
    if (window) {
        gdk_pixbuf_copy_area(window, 0, 0, window_w, h, preview, 10, theme->title_height + 5);
        g_object_unref(window);
    }

    if (menu) {
        gdk_pixbuf_copy_area(menu, 0, 0, menu_w, h, preview, 0, 2 * (theme->title_height + 5));
        g_object_unref(menu);
    }

    RrThemeFree(theme);
    return preview;
}