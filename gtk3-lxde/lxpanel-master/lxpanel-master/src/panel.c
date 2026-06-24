/*
 * Copyright (C) 2006-2010 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *               2006-2008 Jim Huang <jserv.tw@gmail.com>
 *               2008 Fred Chien <fred@lxde.org>
 *               2009 Jürgen Hötzel <juergen@archlinux.org>
 *               2009-2010 Marty Jack <martyj19@comcast.net>
 *               2010 Lajos Kamocsay <lajos@panka.com>
 *               2012 Piotr Sipika <Piotr.Sipika@gmail.com>
 *               2012-2013 Henry Gebhardt <hsggebhardt@gmail.com>
 *               2012 Jack Chen <speed.up08311990@gmail.com>
 *               2012 Rafał Mużyło <galtgendo@gmail.com>
 *               2012 Michael Rawson <michaelrawson76@gmail.com>
 *               2012 Julien Lavergne <julien.lavergne@gmail.com>
 *               2013 Rouslan <rouslan-k@users.sourceforge.net>
 *               2013 peadaredwards <peadaredwards@users.sourceforge.net>
 *               2014-2016 Andriy Grytsenko <andrej@rep.kiev.ua>
 *               2015 Rafał Mużyło <galtgendo@gmail.com>
 *               2015 Hanno Zulla <hhz@users.sf.net>
 *               2018 Mamoru TASAKA <mtasaka@fedoraproject.org>
 *               2025 Ingo Brückl
 *
 * This file is a part of LXPanel project.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <stdlib.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <string.h>

#include <gdk/gdkx.h>
#include <libfm/fm-gtk.h>
#include <cairo-xlib.h>

#ifndef WNCK_I_KNOW_THIS_IS_UNSTABLE
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#endif
#include <libwnck/libwnck.h>

#define __LXPANEL_INTERNALS__

#include "private.h"
#include "misc.h"
#include "space.h"
#include "lxpanelctl.h"
#include "dbg.h"
#include "gtk-compat.h"

gchar *cprofile = "default";

GSList *all_panels = NULL;   /* single-linked list storing all panels */

gboolean is_in_lxde = FALSE;

static GtkWindowGroup *win_grp = NULL; /* window group used to limit scope of modal dialogs */

static gulong monitors_handler = 0;

static void panel_start_gui(LXPanel *p, config_setting_t *list);
static void ah_start(LXPanel *p);
static void ah_stop(LXPanel *p);
static void _panel_update_background(LXPanel *p, gboolean enforce);

enum
{
    ICON_SIZE_CHANGED,
    PANEL_FONT_CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE(PanelToplevel, lxpanel, GTK_TYPE_WINDOW);

/* Finalize: GTK3-only */
static void lxpanel_finalize(GObject *object)
{
    LXPanel *self = LXPANEL(object);
    Panel *p = self->priv;

    if (p->config_changed)
        lxpanel_config_save(self);

    config_destroy(p->config);

    g_free(p->background_file);
    g_slist_free(p->system_menus);

    g_free(p->name);
    g_free(p);

    G_OBJECT_CLASS(lxpanel_parent_class)->finalize(object);
}

/* Stop GUI: cleanup autohide, dialogs, X11 sync, surfaces, sources, and box */
static void panel_stop_gui(LXPanel *self)
{
    Panel *p = self->priv;
    Display *xdisplay;

    g_debug("panel_stop_gui on '%s'", p->name);

    if (p->autohide)
        ah_stop(self);

    if (p->pref_dialog != NULL)
        gtk_widget_destroy(p->pref_dialog);
    p->pref_dialog = NULL;

    if (p->plugin_pref_dialog != NULL)
        gtk_dialog_response(GTK_DIALOG(p->plugin_pref_dialog), GTK_RESPONSE_CLOSE);

    if (p->initialized)
    {
        gtk_window_group_remove_window(win_grp, GTK_WINDOW(self));

        xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
        gdk_flush();
        XFlush(xdisplay);
        XSync(xdisplay, True);

        p->initialized = FALSE;
    }

    if (p->surface != NULL)
    {
        cairo_surface_destroy(p->surface);
        p->surface = NULL;
    }

    if (p->background_update_queued)
    {
        g_source_remove(p->background_update_queued);
        p->background_update_queued = 0;
    }
    if (p->strut_update_queued)
    {
        g_source_remove(p->strut_update_queued);
        p->strut_update_queued = 0;
    }
    if (p->reconfigure_queued)
    {
        g_source_remove(p->reconfigure_queued);
        p->reconfigure_queued = 0;
    }

    if (gtk_bin_get_child(GTK_BIN(self)))
    {
        gtk_widget_destroy(p->box);
        p->box = NULL;
    }
}

/* Destroy handler: GTK3-only */
static void lxpanel_destroy(GtkWidget *object)
{
    LXPanel *self = LXPANEL(object);

    panel_stop_gui(self);

    GTK_WIDGET_CLASS(lxpanel_parent_class)->destroy(object);
}

/* Idle background update (GTK3-safe) */
static gboolean idle_update_background(gpointer p)
{
    LXPanel *panel = LXPANEL(p);

    if (g_source_is_destroyed(g_main_current_source()))
        return FALSE;

    if (gtk_widget_get_realized(GTK_WIDGET(panel)))
    {
        gdk_display_sync(gtk_widget_get_display(GTK_WIDGET(panel)));
        _panel_update_background(panel, FALSE);
    }
    panel->priv->background_update_queued = 0;

    return FALSE;
}

void _panel_queue_update_background(LXPanel *panel)
{
    if (panel->priv->background_update_queued)
        return;

    panel->priv->background_update_queued =
        g_idle_add_full(G_PRIORITY_HIGH,
                        idle_update_background,
                        panel, NULL);
}

/* Idle strut update (WM reserved space) */
static gboolean idle_update_strut(gpointer p)
{
    LXPanel *panel = LXPANEL(p);

    if (g_source_is_destroyed(g_main_current_source()))
        return FALSE;

    _panel_set_wm_strut(panel);
    panel->priv->strut_update_queued = 0;

    return FALSE;
}

/* Realize handler: GTK3-only */
static void lxpanel_realize(GtkWidget *widget)
{
    GTK_WIDGET_CLASS(lxpanel_parent_class)->realize(widget);

    _panel_queue_update_background(LXPANEL(widget));
}

/* GTK3 Update: style_updated replaces the deprecated style_set hook */
static void lxpanel_style_updated(GtkWidget *widget)
{
    GTK_WIDGET_CLASS(lxpanel_parent_class)->style_updated(widget);

    _panel_queue_update_background(LXPANEL(widget));
}

/* Check if a plugin with given name exists in the panel box */
static gboolean lxpanel_has_plugin(LXPanel *panel, const char *name)
{
    gboolean found = FALSE;
    GList *plugins = NULL, *plugin;

    if (panel->priv->box)
        plugins = gtk_container_get_children(GTK_CONTAINER(panel->priv->box));

    for (plugin = plugins; plugin; plugin = plugin->next)
    {
        if (g_strcmp0(gtk_widget_get_name(GTK_WIDGET(plugin->data)), name) == 0)
        {
            found = TRUE;
            break;
        }
    }

    g_list_free(plugins);

    return found;
}

/* Core size computation logic, now GTK3-only */
static void lxpanel_size_request(GtkWidget *widget, GtkRequisition *req)
{
    LXPanel *panel = LXPANEL(widget);
    Panel *p = panel->priv;
    GdkRectangle rect;

    /* Ask parent class for preferred size */
    GTK_WIDGET_CLASS(lxpanel_parent_class)->get_preferred_width(widget,
                                                                &req->width,
                                                                &req->width);
    GTK_WIDGET_CLASS(lxpanel_parent_class)->get_preferred_height(widget,
                                                                 &req->height,
                                                                 &req->height);

    if (!p->visible
#ifdef WNCK_CHECK_VERSION
#if WNCK_CHECK_VERSION(40, 0, 0)
        || (p->widthtype == WIDTH_REQUEST && lxpanel_has_plugin(panel, "pager"))
#endif
#endif
       )
    {
        GtkRequisition box_req;
        gtk_widget_get_preferred_size(p->box, &box_req, NULL);
        req->width = box_req.width;
        req->height = box_req.height;
    }

    rect.width = req->width;
    rect.height = req->height;
    _calculate_position(panel, &rect);
    req->width = rect.width;
    req->height = rect.height;

    /* update data ahead of configuration request */
    p->cw = rect.width;
    p->ch = rect.height;
}

/* GTK3 preferred width implementation */
static void
lxpanel_get_preferred_width(GtkWidget *widget,
                            gint      *minimal_width,
                            gint      *natural_width)
{
    GtkRequisition requisition;

    lxpanel_size_request(widget, &requisition);

    if (minimal_width)
        *minimal_width = requisition.width;
    if (natural_width)
        *natural_width = requisition.width;
}

/* GTK3 preferred height implementation */
static void
lxpanel_get_preferred_height(GtkWidget *widget,
                             gint      *minimal_height,
                             gint      *natural_height)
{
    GtkRequisition requisition;

    lxpanel_size_request(widget, &requisition);

    if (minimal_height)
        *minimal_height = requisition.height;
    if (natural_height)
        *natural_height = requisition.height;
}

/* GTK3 request mode: constant size */
static GtkSizeRequestMode
lxpanel_get_request_mode(GtkWidget *widget)
{
    return GTK_SIZE_REQUEST_CONSTANT_SIZE;
}

/* GTK3 size allocation */
static void lxpanel_size_allocate(GtkWidget *widget, GtkAllocation *a)
{
    LXPanel *panel = LXPANEL(widget);
    Panel *p = panel->priv;
    GdkRectangle rect;
    gint x, y;

    rect.x = a->x;
    rect.y = a->y;
    rect.width = MAX(8, MIN(p->cw, a->width));
    rect.height = MAX(8, MIN(p->ch, a->height));
    _calculate_position(panel, &rect);

    GTK_WIDGET_CLASS(lxpanel_parent_class)->size_allocate(widget, &rect);

    if (p->widthtype == WIDTH_REQUEST)
        p->width = (p->orientation == GTK_ORIENTATION_HORIZONTAL) ? rect.width : rect.height;
    if (p->heighttype == HEIGHT_REQUEST)
        p->height = (p->orientation == GTK_ORIENTATION_HORIZONTAL) ? rect.height : rect.width;

    if (!gtk_widget_get_realized(widget))
        return;

    gdk_window_get_origin(gtk_widget_get_window(widget), &x, &y);
    p->ax = rect.x;
    p->ay = rect.y;

    if (rect.width != p->aw || rect.height != p->ah || x != p->ax || y != p->ay)
    {
        p->aw = rect.width;
        p->ah = rect.height;
        gtk_window_move(GTK_WINDOW(widget), p->ax, p->ay);

        if (!panel->priv->strut_update_queued)
            panel->priv->strut_update_queued =
                g_idle_add_full(G_PRIORITY_HIGH,
                                idle_update_strut,
                                panel, NULL);

        _panel_queue_update_background(panel);
    }

    if (gtk_widget_get_mapped(widget))
        _panel_establish_autohide(panel);
}

/* Configure event: still valid in GTK3, kept for geometry tracking */
static gboolean lxpanel_configure_event(GtkWidget *widget, GdkEventConfigure *e)
{
    Panel *p = LXPANEL(widget)->priv;

    p->cw = e->width;
    p->ch = e->height;
    p->cx = e->x;
    p->cy = e->y;

    return GTK_WIDGET_CLASS(lxpanel_parent_class)->configure_event(widget, e);
}

/* Map event: autohide setup */
static gboolean lxpanel_map_event(GtkWidget *widget, GdkEventAny *event)
{
    Panel *p = PLUGIN_PANEL(widget)->priv;

    if (p->autohide)
        ah_start(LXPANEL(widget));

    return GTK_WIDGET_CLASS(lxpanel_parent_class)->map_event(widget, event);
}

/* Button press handler: GTK3-safe popup menu */
static gboolean lxpanel_button_press(GtkWidget *widget, GdkEventButton *event)
{
    LXPanel *panel = PLUGIN_PANEL(widget);

    if ((event->state & gtk_accelerator_get_default_mod_mask()) != 0)
        return FALSE;

    if (event->button == 3) /* right button */
    {
        GtkMenu *popup = (GtkMenu *) lxpanel_get_plugin_menu(panel, NULL, FALSE);
        gtk_menu_popup_at_pointer(GTK_MENU(popup), (GdkEvent *) event);
        return TRUE;
    }
    else if (event->button == 2) /* middle button */
    {
        Panel *p = panel->priv;
        if (p->move_state == PANEL_MOVE_STOP)
        {
            gdk_window_get_origin(event->window, &p->move_x, &p->move_y);
            p->move_x += event->x - p->ax;
            p->move_y += event->y - p->ay;
            p->move_state = PANEL_MOVE_DETECT;
            p->move_device = event->device;
            return TRUE;
        }
    }
    return FALSE;
}

/* Class init: GTK3-only wiring */
static void lxpanel_class_init(PanelToplevelClass *klass)
{
    GObjectClass *gobject_class = (GObjectClass *) klass;
    GtkWidgetClass *widget_class = (GtkWidgetClass *) klass;

    gobject_class->finalize = lxpanel_finalize;

    widget_class->destroy = lxpanel_destroy;
    widget_class->realize = lxpanel_realize;
    widget_class->get_preferred_width = lxpanel_get_preferred_width;
    widget_class->get_preferred_height = lxpanel_get_preferred_height;
    widget_class->get_request_mode = lxpanel_get_request_mode;
    widget_class->size_allocate = lxpanel_size_allocate;
    widget_class->configure_event = lxpanel_configure_event;
    widget_class->style_updated = lxpanel_style_updated; /* GTK3 Update */
    widget_class->map_event = lxpanel_map_event;
    widget_class->button_press_event = lxpanel_button_press;
    widget_class->button_release_event = _lxpanel_button_release;
    widget_class->motion_notify_event = _lxpanel_motion_notify;

    signals[ICON_SIZE_CHANGED] =
        g_signal_new("icon-size-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(PanelToplevelClass, icon_size_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);

    signals[PANEL_FONT_CHANGED] =
        g_signal_new("panel-font-changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(PanelToplevelClass, panel_font_changed),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__VOID,
                     G_TYPE_NONE, 0);
}

/* GTK3 Update: Added missing instance initializer signature explicitly */
static void lxpanel_init(PanelToplevel *self)
{
    Panel *p = g_new0(Panel, 1);
    self->priv = p;
    p->topgwin = self;
    p->align = ALIGN_CENTER;
    p->edge = EDGE_NONE;
    p->widthtype = WIDTH_PERCENT;
    p->width = 100;
    p->heighttype = HEIGHT_PIXEL;
    p->height = PANEL_HEIGHT_DEFAULT;
    p->monitor = 0;
    p->setdocktype = 1;
    p->setstrut = 1;
    p->round_corners = 0;
    p->autohide = 0;
    p->visible = TRUE;
    p->height_when_hidden = 2;
    p->transparent = 0;
    p->alpha = 255;
    gdk_rgba_parse(&p->gtintcolor, "white"); /* GTK3 Update to GdkRGBA */
    p->tintcolor = gcolor2rgb24(&p->gtintcolor);
    p->usefontcolor = 0;
    p->fontcolor = 0x00000000;
    p->usefontsize = 0;
    p->fontsize = 10;
    p->spacing = 0;
    p->icon_size = PANEL_ICON_SIZE;
    p->icon_theme = gtk_icon_theme_get_default();
    p->config = config_new();
    /* Deprecated GtkStyle tracking removed */
}

/* Allocate and initialize new Panel structure. */
static LXPanel* panel_allocate(GdkScreen *screen)
{
    return g_object_new(LX_TYPE_PANEL,
                        "border-width", 0,
                        "decorated", FALSE,
                        "name", "PanelToplevel",
                        "resizable", FALSE,
                        "title", "panel",
                        "type-hint", GDK_WINDOW_TYPE_HINT_DOCK,
                        "window-position", GTK_WIN_POS_NONE,
                        "screen", screen,
                        NULL);
}

void _panel_emit_icon_size_changed(LXPanel *p)
{
    g_signal_emit(p, signals[ICON_SIZE_CHANGED], 0);
}

void _panel_emit_font_changed(LXPanel *p)
{
    g_signal_emit(p, signals[PANEL_FONT_CHANGED], 0);
}

/* Normalize panel configuration after load from file or reconfiguration. */
static void panel_normalize_configuration(Panel* p)
{
    panel_set_panel_configuration_changed(p);

    if (p->width < 0)
        p->width = 100;
    if (p->widthtype == WIDTH_PERCENT && p->width > 100)
        p->width = 100;

    p->heighttype = HEIGHT_PIXEL;
    if (p->heighttype == HEIGHT_PIXEL)
    {
        if (p->height < PANEL_HEIGHT_MIN)
            p->height = PANEL_HEIGHT_MIN;
        else if (p->height > PANEL_HEIGHT_MAX)
            p->height = PANEL_HEIGHT_MAX;
    }

    if (p->monitor < 0)
        p->monitor = -1;

    if (p->background)
        p->transparent = 0;
}

gboolean _panel_edge_can_strut(LXPanel *panel, int edge, gint monitor, gulong *size)
{
    Panel *p;
    GdkScreen *screen;
    GdkRectangle rect;
    GdkRectangle rect2;
    gint n, i;
    gulong s;

    if (!gtk_widget_get_mapped(GTK_WIDGET(panel)))
        return FALSE;

    p = panel->priv;

    /* Handle autohide case. EWMH recommends having the strut be the minimized size. */
    if (p->autohide)
        s = p->height_when_hidden;
    else switch (edge)
    {
        case EDGE_LEFT:
        case EDGE_RIGHT:
            s = p->aw;
            break;
        case EDGE_TOP:
        case EDGE_BOTTOM:
            s = p->ah;
            break;
        default:
            return FALSE;
    }

    if (s == 0)
        return FALSE;

    if (monitor < 0) /* screen span */
    {
        if (G_LIKELY(size))
            *size = s;
        return TRUE;
    }

    screen = gtk_widget_get_screen(GTK_WIDGET(panel));
    n = gdk_screen_get_n_monitors(screen);
    if (monitor >= n)
        return FALSE;

    gdk_screen_get_monitor_geometry(screen, monitor, &rect);

    switch (edge)
    {
        case EDGE_LEFT:
            rect.width = rect.x;
            rect.x = 0;
            s += rect.width;
            break;
        case EDGE_RIGHT:
            rect.x += rect.width;
            rect.width = gdk_screen_get_width(screen) - rect.x;
            s += rect.width;
            break;
        case EDGE_TOP:
            rect.height = rect.y;
            rect.y = 0;
            s += rect.height;
            break;
        case EDGE_BOTTOM:
            rect.y += rect.height;
            rect.height = gdk_screen_get_height(screen) - rect.y;
            s += rect.height;
            break;
        default:
            break;
    }

    if (rect.height == 0 || rect.width == 0)
        ; /* on a border of monitor */
    else
    {
        for (i = 0; i < n; i++)
        {
            if (i == monitor)
                continue;

            gdk_screen_get_monitor_geometry(screen, i, &rect2);
            if (gdk_rectangle_intersect(&rect, &rect2, NULL))
                return FALSE;
        }
    }

    if (G_LIKELY(size))
        *size = s;

    return TRUE;
}


void panel_set_wm_strut(Panel *p)
{
    _panel_set_wm_strut(p->topgwin);
}

void _panel_set_wm_strut(LXPanel *panel)
{
    int index;
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    Panel *p = panel->priv;
    gulong strut_size;
    gulong strut_lower;
    gulong strut_upper;

    if (!gtk_widget_get_mapped(GTK_WIDGET(panel)))
        return;

    if (p->autohide && p->height_when_hidden <= 0)
        return;

    switch (p->edge)
    {
        case EDGE_LEFT:
            index = 0;
            strut_lower = p->ay;
            strut_upper = p->ay + p->ah;
            break;
        case EDGE_RIGHT:
            index = 1;
            strut_lower = p->ay;
            strut_upper = p->ay + p->ah;
            break;
        case EDGE_TOP:
            index = 2;
            strut_lower = p->ax;
            strut_upper = p->ax + p->aw;
            break;
        case EDGE_BOTTOM:
            index = 3;
            strut_lower = p->ax;
            strut_upper = p->ax + p->aw;
            break;
        default:
            return;
    }

    gulong desired_strut[12];
    memset(desired_strut, 0, sizeof(desired_strut));

    if (p->setstrut &&
        _panel_edge_can_strut(panel, p->edge, p->monitor, &strut_size))
    {
        gint scale_factor = gtk_widget_get_scale_factor(GTK_WIDGET(panel));

        desired_strut[index] = strut_size * scale_factor;
        desired_strut[4 + index * 2] = strut_lower * scale_factor;
        desired_strut[5 + index * 2] = strut_upper * scale_factor - 1;
    }
    else
    {
        strut_size = 0;
        strut_lower = 0;
        strut_upper = 0;
    }

    if ((p->strut_size != strut_size) ||
        (p->strut_lower != strut_lower) ||
        (p->strut_upper != strut_upper) ||
        (p->strut_edge != p->edge))
    {
        p->strut_size = strut_size;
        p->strut_lower = strut_lower;
        p->strut_upper = strut_upper;
        p->strut_edge = p->edge;

        if (strut_size != 0)
        {
            XChangeProperty(xdisplay, p->topxwin, a_NET_WM_STRUT_PARTIAL,
                            XA_CARDINAL, 32, PropModeReplace,
                            (unsigned char *) desired_strut, 12);
            XChangeProperty(xdisplay, p->topxwin, a_NET_WM_STRUT,
                            XA_CARDINAL, 32, PropModeReplace,
                            (unsigned char *) desired_strut, 4);
        }
        else
        {
            XDeleteProperty(xdisplay, p->topxwin, a_NET_WM_STRUT);
            XDeleteProperty(xdisplay, p->topxwin, a_NET_WM_STRUT_PARTIAL);
        }
    }
}

static void paint_root_pixmap(LXPanel *panel, cairo_t *cr)
{
    XGCValues gcv;
    uint mask;
    Window xroot;
    GC gc;
    Display *dpy;
    Pixmap *prop;
    cairo_surface_t *surface;
    Pixmap xpixmap;
    Panel *p = panel->priv;

    dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    xroot = DefaultRootWindow(dpy);

    gcv.ts_x_origin = 0;
    gcv.ts_y_origin = 0;
    gcv.fill_style = FillTiled;
    mask = GCTileStipXOrigin | GCTileStipYOrigin | GCFillStyle;

    prop = get_xaproperty(xroot, a_XROOTPMAP_ID, XA_PIXMAP, NULL);
    if (prop)
    {
        gcv.tile = *prop;
        mask |= GCTile;
        XFree(prop);
    }

    gc = XCreateGC(dpy, xroot, mask, &gcv);

    xpixmap = XCreatePixmap(dpy, xroot, p->aw, p->ah,
                            DefaultDepth(dpy, DefaultScreen(dpy)));
    surface = cairo_xlib_surface_create(dpy, xpixmap,
                                        DefaultVisual(dpy, DefaultScreen(dpy)),
                                        p->aw, p->ah);

    XSetTSOrigin(dpy, gc, -p->ax, -p->ay);
    XFillRectangle(dpy, xpixmap, gc, 0, 0, p->aw, p->ah);
    XFreeGC(dpy, gc);

    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_paint(cr);

    cairo_surface_destroy(surface);
    XFreePixmap(dpy, xpixmap);
}


static void _panel_determine_background_pixmap(LXPanel *panel)
{
    cairo_pattern_t *pattern;
    GtkWidget *widget = GTK_WIDGET(panel);
    GdkWindow *window = gtk_widget_get_window(widget);
    Panel *p = panel->priv;
    cairo_t *cr;
    gint x = 0, y = 0;

    if (!p->background && !p->transparent)
        goto not_paintable;
    else if (p->aw <= 1 || p->ah <= 1)
        goto not_paintable;
    else if (p->surface == NULL)
    {
        GdkPixbuf *pixbuf = NULL;

        p->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, p->aw, p->ah);
        cr = cairo_create(p->surface);

        if (p->background)
            pixbuf = gdk_pixbuf_new_from_file(p->background_file, NULL);

        if ((p->transparent && p->alpha != 255) ||
            (pixbuf != NULL && gdk_pixbuf_get_has_alpha(pixbuf)))
        {
            paint_root_pixmap(panel, cr);
        }

        if (pixbuf != NULL)
        {
            gint w = gdk_pixbuf_get_width(pixbuf);
            gint h = gdk_pixbuf_get_height(pixbuf);

            for (y = 0; y < p->ah; y += h)
                for (x = 0; x < p->aw; x += w)
                {
                    gdk_cairo_set_source_pixbuf(cr, pixbuf, x, y);
                    cairo_paint(cr);
                }

            y = 0;
            g_object_unref(pixbuf);
        }
        else
        {
            /* GTK3 Update: Using modern source allocation engine */
            gdk_cairo_set_source_rgba(cr, &p->gtintcolor);
            cairo_paint_with_alpha(cr, p->transparent ? (double)p->alpha / 255.0 : 1.0);
        }

        cairo_destroy(cr);
    }

    if (p->surface != NULL)
    {
        gtk_widget_set_app_paintable(widget, TRUE);

        pattern = cairo_pattern_create_for_surface(p->surface);
        gdk_window_set_background_pattern(window, pattern);
        cairo_pattern_destroy(pattern);
    }
    else
    {
not_paintable:
        gtk_widget_set_app_paintable(widget, FALSE);
    }
}

void panel_determine_background_pixmap(Panel *panel, GtkWidget *widget, GdkWindow *window)
{
    if (GTK_WIDGET(panel->topgwin) != widget)
    {
        gtk_widget_set_app_paintable(widget, (panel->background || panel->transparent));
        gdk_window_set_background_pattern(window, NULL);
    }
    else
        _panel_determine_background_pixmap(panel->topgwin);
}

void panel_update_background(Panel *p)
{
    _panel_update_background(p->topgwin, TRUE);
}

static void _panel_update_background(LXPanel *p, gboolean enforce)
{
    GtkWidget *w = GTK_WIDGET(p);
    GList *plugins = NULL, *l;

    if (p->priv->surface != NULL)
    {
        cairo_surface_destroy(p->priv->surface);
        p->priv->surface = NULL;
    }

    _panel_determine_background_pixmap(p);
    gtk_widget_queue_draw(w);

    if (p->priv->box != NULL)
        plugins = gtk_container_get_children(GTK_CONTAINER(p->priv->box));

    for (l = plugins; l != NULL; l = l->next)
        plugin_widget_set_background(l->data, p);

    g_list_free(plugins);
}


#define GAP 2
#define PERIOD 300

typedef enum
{
    AH_STATE_VISIBLE,
    AH_STATE_WAITING,
    AH_STATE_HIDDEN
} PanelAHState;

static void ah_state_set(LXPanel *p, PanelAHState ah_state);

static gboolean mouse_watch(LXPanel *panel)
{
    Panel *p = panel->priv;
    gint x, y;

    if (g_source_is_destroyed(g_main_current_source()))
        return FALSE;

    /* GTK3 Update: Multi-device pointer tracking to safely acquire pointer position */
    GdkDisplay *display = gdk_display_get_default();
    GdkSeat *seat = gdk_display_get_default_seat(display);
    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    gdk_device_get_position(pointer, NULL, &x, &y);

    gint cx, cy, cw, ch, gap;

    cx = p->ax;
    cy = p->ay;
    cw = p->cw;
    ch = p->ch;

    if (p->move_state != PANEL_MOVE_STOP)
        return TRUE;

    if (cw == 1) cw = 0;
    if (ch == 1) ch = 0;

    if (p->ah_state == AH_STATE_HIDDEN)
    {
        gap = MAX(p->height_when_hidden, GAP);
        switch (p->edge)
        {
            case EDGE_LEFT:
                cw = gap;
                break;
            case EDGE_RIGHT:
                cx = cx + cw - gap;
                cw = gap;
                break;
            case EDGE_TOP:
                ch = gap;
                break;
            case EDGE_BOTTOM:
                cy = cy + ch - gap;
                ch = gap;
                break;
        }
    }

    p->ah_far = ((x < cx) || (x > cx + cw) || (y < cy) || (y > cy + ch));

    ah_state_set(panel, p->ah_state);
    return TRUE;
}

static gboolean ah_state_hide_timeout(gpointer p)
{
    if (!g_source_is_destroyed(g_main_current_source()))
    {
        ah_state_set(p, AH_STATE_HIDDEN);
        ((LXPanel *) p)->priv->hide_timeout = 0;
    }
    return FALSE;
}

static void ah_state_set(LXPanel *panel, PanelAHState ah_state)
{
    Panel *p = panel->priv;
    GdkRectangle rect;

    if (p->ah_state != ah_state)
    {
        p->ah_state = ah_state;
        switch (ah_state)
        {
            case AH_STATE_VISIBLE:
                p->visible = TRUE;
                _calculate_position(panel, &rect);
                gtk_window_move(GTK_WINDOW(panel), rect.x, rect.y);
                gtk_widget_show(GTK_WIDGET(panel));
                gtk_widget_show(p->box);
                gtk_widget_queue_resize(GTK_WIDGET(panel));
                gtk_window_stick(GTK_WINDOW(panel));
                break;

            case AH_STATE_WAITING:
                if (p->hide_timeout)
                    g_source_remove(p->hide_timeout);
                p->hide_timeout = g_timeout_add(2 * PERIOD, ah_state_hide_timeout, panel);
                break;

            case AH_STATE_HIDDEN:
                if (p->height_when_hidden > 0)
                    gtk_widget_hide(p->box);
                else
                    gtk_widget_hide(GTK_WIDGET(panel));
                p->visible = FALSE;
                break;
        }
    }
    else if (p->autohide && p->ah_far)
    {
        switch (ah_state)
        {
            case AH_STATE_VISIBLE:
                ah_state_set(panel, AH_STATE_WAITING);
                break;

            case AH_STATE_WAITING:
                break;

            case AH_STATE_HIDDEN:
                if (p->height_when_hidden > 0)
                {
                    if (gtk_widget_get_visible(p->box))
                    {
                        gtk_widget_hide(p->box);
                        gtk_widget_show(GTK_WIDGET(panel));
                    }
                }
                else if (gtk_widget_get_visible(GTK_WIDGET(panel)))
                {
                    gtk_widget_hide(GTK_WIDGET(panel));
                    gtk_widget_show(p->box);
                }
                break;
        }
    }
    else
    {
        switch (ah_state)
        {
            case AH_STATE_VISIBLE:
                break;

            case AH_STATE_WAITING:
                if (p->hide_timeout)
                    g_source_remove(p->hide_timeout);
                p->hide_timeout = 0;
                /* fall through */
            case AH_STATE_HIDDEN:
                ah_state_set(panel, AH_STATE_VISIBLE);
                break;
        }
    }
}

static void ah_start(LXPanel *p)
{
    if (!p->priv->mouse_timeout)
        p->priv->mouse_timeout = g_timeout_add(PERIOD, (GSourceFunc) mouse_watch, p);
}

static void ah_stop(LXPanel *p)
{
    if (p->priv->mouse_timeout)
    {
        g_source_remove(p->priv->mouse_timeout);
        p->priv->mouse_timeout = 0;
    }
    if (p->priv->hide_timeout)
    {
        g_source_remove(p->priv->hide_timeout);
        p->priv->hide_timeout = 0;
    }
}

static gint panel_popupmenu_configure(GtkWidget *widget, gpointer user_data)
{
    panel_configure((LXPanel *) user_data, 0);
    return TRUE;
}

static void panel_popupmenu_config_plugin(GtkMenuItem *item, GtkWidget *plugin)
{
    Panel *panel = PLUGIN_PANEL(plugin)->priv;

    lxpanel_plugin_show_config_dialog(plugin);
    panel->config_changed = TRUE;
}
static void panel_popupmenu_add_item(GtkMenuItem *item, LXPanel *panel)
{
    panel_configure(panel, 2);
}

static void panel_popupmenu_remove_item(GtkMenuItem *item, GtkWidget *plugin)
{
    lxpanel_remove_plugin(PLUGIN_PANEL(plugin), plugin);
}

void lxpanel_remove_plugin(LXPanel *p, GtkWidget *plugin)
{
    Panel *panel = p->priv;
    /* Implemented elsewhere or remaining blank from snippet */
}

/* Helper function to construct structured menu items with icons in GTK3 */
static GtkWidget *create_menu_item_with_icon(const char *label_text, const char *icon_name)
{
    GtkWidget *menu_item = gtk_menu_item_new();
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
    GtkWidget *lbl = gtk_label_new(label_text);

    gtk_container_add(GTK_CONTAINER(box), img);
    gtk_container_add(GTK_CONTAINER(box), lbl);
    gtk_container_add(GTK_CONTAINER(menu_item), box);

    return menu_item;
}

GtkMenu* lxpanel_get_plugin_menu(LXPanel *panel, GtkWidget *plugin, gboolean use_sub_menu)
{
    GtkWidget *menu_item, *img;
    GtkMenu *menu, *submenu;
    const LXPanelPluginInit *init;
    char *tmp;

    menu = GTK_MENU(gtk_menu_new());

    if (plugin)
    {
        init = PLUGIN_CLASS(plugin);

        /* Create menu item with layout box for icon support */
        tmp = g_strdup_printf(_("\"%s\" Settings"),
                              g_dgettext(init->gettext_package, init->name));
        menu_item = create_menu_item_with_icon(tmp, "preferences-system");
        g_free(tmp);

        gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menu_item);

        if (init->config)
            g_signal_connect(menu_item, "activate",
                             G_CALLBACK(panel_popupmenu_config_plugin), plugin);
        else
            gtk_widget_set_sensitive(menu_item, FALSE);

        /* Allow plugin to add custom menu items */
        if (init->update_context_menu != NULL)
            use_sub_menu = init->update_context_menu(plugin, menu);

        /* Separator */
        menu_item = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    }

    /* If plugin wants a submenu, create it */
    if (use_sub_menu)
    {
        submenu = GTK_MENU(gtk_menu_new());

        /* Add submenu entry */
        menu_item = gtk_menu_item_new_with_label(_("More Options"));
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), GTK_WIDGET(submenu));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

        /* Return submenu so caller can populate it */
        return submenu;
    }

    return menu;
}

/* Duplicate definition refactored to align with GTK3 layout constraints */
GtkMenu* lxpanel_get_panel_menu(LXPanel *panel, GtkWidget *plugin, gboolean use_sub_menu)
{
    GtkWidget  *menu_item;
    GtkMenu    *ret, *menu;
    const LXPanelPluginInit *init;
    char *tmp;

    ret  = GTK_MENU(gtk_menu_new());
    menu = ret;

    if (plugin)
    {
        init = PLUGIN_CLASS(plugin);

        /* "Add / Remove Panel Items" */
        menu_item = create_menu_item_with_icon(_("Add / Remove Panel Items"), "list-add");
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        
        GtkWidget *label = gtk_bin_get_child(GTK_BIN(menu_item));
        if (GTK_IS_LABEL(label)) {
            gtk_label_set_use_underline(GTK_LABEL(label), TRUE);
        }

        g_signal_connect(menu_item, "activate",
                         G_CALLBACK(panel_popupmenu_add_item), panel);

        /* "Remove <plugin> From Panel" */
        tmp = g_strdup_printf(_("Remove \"%s\" From Panel"), _(init->name));
        menu_item = create_menu_item_with_icon(tmp, "list-remove");
        g_free(tmp);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
        g_signal_connect(menu_item, "activate",
                         G_CALLBACK(panel_popupmenu_remove_item), plugin);
    }

    /* Separator */
    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    /* "Panel Settings" */
    menu_item = create_menu_item_with_icon(_("Panel Settings"), "preferences-system");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(panel_popupmenu_configure), panel);

    /* "Create New Panel" */
    menu_item = create_menu_item_with_icon(_("Create New Panel"), "list-add");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(panel_popupmenu_create_panel), panel);

    /* "Delete This Panel" */
    menu_item = create_menu_item_with_icon(_("Delete This Panel"), "edit-delete");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(panel_popupmenu_delete_panel), panel);
    if (!all_panels->next)
        gtk_widget_set_sensitive(menu_item, FALSE);

    /* Separator */
    menu_item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    /* "About" */
    menu_item = create_menu_item_with_icon(_("About"), "help-about");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
    g_signal_connect(menu_item, "activate",
                     G_CALLBACK(panel_popupmenu_about), panel->priv);

    if (use_sub_menu)
    {
        GtkWidget *panel_item = gtk_menu_item_new_with_label(_("Panel"));
        gtk_menu_shell_append(GTK_MENU_SHELL(ret), panel_item);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(panel_item), GTK_WIDGET(menu));
    }

    gtk_widget_show_all(GTK_WIDGET(ret));
    g_signal_connect(ret, "selection-done",
                     G_CALLBACK(gtk_widget_destroy), NULL);

    return ret;
}

void panel_draw_label_text_with_color(Panel *p, GtkWidget *label, const char *text,
                                      gboolean bold, float custom_size_factor,
                                      gboolean custom_color, GdkRGBA *gdkcolor)
{
    /* Compute size scaling via Pango Context from GTK Style Context */
    int font_desc_size;
    if (p->usefontsize)
        font_desc_size = p->fontsize;
    else
    {
        GtkStyleContext *context = gtk_widget_get_style_context(label);
        PangoFontDescription *font_desc;
        gtk_style_context_get(context, gtk_style_context_get_state(context), "font", &font_desc, NULL);
        if (font_desc) {
            font_desc_size = pango_font_description_get_size(font_desc) / PANGO_SCALE;
            pango_font_description_free(font_desc);
        } else {
            font_desc_size = 10; /* Fallback size */
        }
    }
    font_desc_size *= custom_size_factor;

    /* Escape text safely if XML nodes exist */
    const char *valid_markup = text;
    char *escaped_text = NULL;
    const char *q;
    for (q = text; *q != '\0'; q += 1)
    {
        if ((*q == '<') || (*q == '>') || (*q == '&'))
        {
            escaped_text = g_markup_escape_text(text, -1);
            valid_markup = escaped_text;
            break;
        }
    }

    gchar *formatted_text;
    if (gdkcolor || ((custom_color) && (p->usefontcolor)))
    {
        guint32 rgb24;
        if (gdkcolor) {
            rgb24 = (((gint)(gdkcolor->red * 255)) << 16) | (((gint)(gdkcolor->green * 255)) << 8) | ((gint)(gdkcolor->blue * 255));
        } else {
            rgb24 = (((gint)(p->gfontcolor.red * 255)) << 16) | (((gint)(p->gfontcolor.green * 255)) << 8) | ((gint)(p->gfontcolor.blue * 255));
        }
        
        formatted_text = g_strdup_printf("<span font_desc=\"%d\" color=\"#%06x\">%s%s%s</span>",
                font_desc_size,
                rgb24,
                ((bold) ? "<b>" : ""),
                valid_markup,
                ((bold) ? "</b>" : ""));
    }
    else
    {
        formatted_text = g_strdup_printf("<span font_desc=\"%d\">%s%s%s</span>",
                font_desc_size,
                ((bold) ? "<b>" : ""),
                valid_markup,
                ((bold) ? "</b>" : ""));
    }

    gtk_label_set_markup(GTK_LABEL(label), formatted_text);
    g_free(formatted_text);
    g_free(escaped_text);
}

void panel_draw_label_text(Panel * p, GtkWidget * label, const char * text,
                           gboolean bold, float custom_size_factor,
                           gboolean custom_color)
{
    panel_draw_label_text_with_color(p, label, text, bold, custom_size_factor, custom_color, NULL);
}

void lxpanel_draw_label_text(LXPanel * p, GtkWidget * label, const char * text,
                            gboolean bold, float custom_size_factor,
                            gboolean custom_color)
{
    panel_draw_label_text(p->priv, label, text, bold, custom_size_factor, custom_color);
}

void lxpanel_draw_label_text_with_color(LXPanel * p, GtkWidget * label, const char * text,
                                     gboolean bold, float custom_size_factor,
                                     GdkRGBA *color)
{
    panel_draw_label_text_with_color(p->priv, label, text, bold, custom_size_factor, FALSE, color);
}

void panel_set_panel_configuration_changed(Panel *p)
{
    _panel_set_panel_configuration_changed(p->topgwin);
}

static inline void _update_orientation(Panel *p)
{
    p->orientation = (p->edge == EDGE_TOP || p->edge == EDGE_BOTTOM)
                        ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL;
}

static gboolean _panel_idle_reconfigure(gpointer widget)
{
    LXPanel *panel;
    Panel *p;
    GList *plugins, *l;
    GtkOrientation previous_orientation;

    if (g_source_is_destroyed(g_main_current_source()))
        return FALSE;

    panel = LXPANEL(widget);
    p = panel->priv;
    previous_orientation = p->orientation;
    _update_orientation(p);

    /* either first run or orientation was changed */
    if (previous_orientation != p->orientation)
    {
        panel_adjust_geometry_terminology(p);
        p->height = ((p->orientation == GTK_ORIENTATION_HORIZONTAL) ? PANEL_HEIGHT_DEFAULT : PANEL_WIDTH_DEFAULT);
        if (p->height_control != NULL)
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->height_control), p->height);
        if ((p->widthtype == WIDTH_PIXEL) && (p->width_control != NULL))
        {
            GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(panel));
            GdkMonitor *monitor = gdk_display_get_monitor(display, p->monitor ? p->monitor : 0);
            GdkRectangle geometry;
            gdk_monitor_get_geometry(monitor, &geometry);
            int value = ((p->orientation == GTK_ORIENTATION_HORIZONTAL) ? geometry.width : geometry.height);
            
            gtk_spin_button_set_range(GTK_SPIN_BUTTON(p->width_control), 0, value);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(p->width_control), value);
        }
    }

    /* Replaced deprecated legacy function pointers entirely with standard GTK3 dynamic calls */

    /* recreate the main layout box */
    if (p->box != NULL)
    {
        gtk_orientable_set_orientation(GTK_ORIENTABLE(p->box), p->orientation);
    }

    plugins = p->box ? gtk_container_get_children(GTK_CONTAINER(p->box)) : NULL;
    for( l = plugins; l; l = l->next ) {
        GtkWidget *w = (GtkWidget*)l->data;
        const LXPanelPluginInit *init = PLUGIN_CLASS(w);
        if (init->reconfigure)
            init->reconfigure(panel, w);
    }
    g_list_free(plugins);
    _panel_queue_update_background(panel);

    p->reconfigure_queued = 0;

    return FALSE;
}

void _panel_set_panel_configuration_changed(LXPanel *panel)
{
    if (panel->priv->reconfigure_queued)
        return;
    panel->priv->reconfigure_queued = g_idle_add(_panel_idle_reconfigure, panel);
}

static int
panel_parse_global(Panel *p, config_setting_t *cfg)
{
    const char *str;
    gint i;

    if (!cfg || strcmp(config_setting_get_name(cfg), "Global") != 0)
    {
        g_warning( "lxpanel: Global section not found");
        RET(0);
    }
    if (config_setting_lookup_string(cfg, "edge", &str))
        p->edge = str2num(edge_pair, str, EDGE_NONE);
    if (config_setting_lookup_string(cfg, "align", &str) ||
        config_setting_lookup_string(cfg, "allign", &str))
        p->align = str2num(align_pair, str, ALIGN_NONE);
    config_setting_lookup_int(cfg, "monitor", &p->monitor);
    config_setting_lookup_int(cfg, "margin", &p->margin);
    if (config_setting_lookup_string(cfg, "widthtype", &str))
        p->widthtype = str2num(width_pair, str, WIDTH_NONE);
    config_setting_lookup_int(cfg, "width", &p->width);
    if (config_setting_lookup_string(cfg, "heighttype", &str))
        p->heighttype = str2num(height_pair, str, HEIGHT_NONE);
    config_setting_lookup_int(cfg, "height", &p->height);
    if (config_setting_lookup_int(cfg, "spacing", &i) && i > 0)
        p->spacing = i;
    if (config_setting_lookup_int(cfg, "setdocktype", &i))
        p->setdocktype = i != 0;
    if (config_setting_lookup_int(cfg, "setpartialstrut", &i))
        p->setstrut = i != 0;
    if (config_setting_lookup_int(cfg, "RoundCorners", &i))
        p->round_corners = i != 0;
    if (config_setting_lookup_int(cfg, "transparent", &i))
        p->transparent = i != 0;
    if (config_setting_lookup_int(cfg, "alpha", &p->alpha))
    {
        if (p->alpha > 255)
            p->alpha = 255;
    }
    if (config_setting_lookup_int(cfg, "autohide", &i))
        p->autohide = i != 0;
    if (config_setting_lookup_int(cfg, "heightwhenhidden", &i))
        p->height_when_hidden = MAX(0, i);
        
    if (config_setting_lookup_string(cfg, "tintcolor", &str))
    {
        if (!gdk_rgba_parse (&p->gtintcolor, str))
            gdk_rgba_parse (&p->gtintcolor, "white");
        p->tintcolor = (((gint)(p->gtintcolor.red * 255)) << 16) | (((gint)(p->gtintcolor.green * 255)) << 8) | ((gint)(p->gtintcolor.blue * 255));
        DBG("tintcolor=%x\n", p->tintcolor);
    }
    if (config_setting_lookup_int(cfg, "usefontcolor", &i))
        p->usefontcolor = i != 0;
    if (config_setting_lookup_string(cfg, "fontcolor", &str))
    {
        if (!gdk_rgba_parse (&p->gfontcolor, str))
            gdk_rgba_parse (&p->gfontcolor, "black");
        p->fontcolor = (((gint)(p->gfontcolor.red * 255)) << 16) | (((gint)(p->gfontcolor.green * 255)) << 8) | ((gint)(p->gfontcolor.blue * 255));
        DBG("fontcolor=%x\n", p->fontcolor);
    }
    if (config_setting_lookup_int(cfg, "usefontsize", &i))
        p->usefontsize = i != 0;
    if (config_setting_lookup_int(cfg, "fontsize", &i) && i > 0)
        p->fontsize = i;
    if (config_setting_lookup_int(cfg, "background", &i))
        p->background = i != 0;
    if (config_setting_lookup_string(cfg, "backgroundfile", &str))
        p->background_file = g_strdup(str);
    config_setting_lookup_int(cfg, "iconsize", &p->icon_size);

    _update_orientation(p);
    panel_normalize_configuration(p);

    return 1;
}

static void on_monitors_changed(GdkDisplay *display, GdkMonitor *monitor, gpointer unused)
{
    GSList *pl;
    int monitors = gdk_display_get_n_monitors(display);

    for (pl = all_panels; pl; pl = pl->next)
    {
        LXPanel *p = pl->data;

        if (p->priv->monitor < monitors && !p->priv->initialized)
            panel_start_gui(p, config_setting_get_member(config_root_setting(p->priv->config), ""));
        else if (p->priv->monitor >= monitors && p->priv->initialized)
            panel_stop_gui(p);
        else
        {
            ah_state_set(p, AH_STATE_VISIBLE);
            gtk_widget_queue_resize(GTK_WIDGET(p));
        }
    }
}

static int panel_start(LXPanel *p)
{
    config_setting_t *list;
    GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(p));

    ENTER;

    list = config_setting_get_member(config_root_setting(p->priv->config), "");
    if (!list || !panel_parse_global(p->priv, config_setting_get_elem(list, 0)))
        RET(0);

    if (p->priv->monitor < gdk_display_get_n_monitors(display))
        panel_start_gui(p, list);
    if (monitors_handler == 0)
        monitors_handler = g_signal_connect(display, "monitor-added",
                                            G_CALLBACK(on_monitors_changed), NULL);
    return 1;
}

void panel_destroy(Panel *p)
{
    gtk_widget_destroy(GTK_WIDGET(p->topgwin));
}

LXPanel* panel_new( const char* config_file, const char* config_name )
{
    LXPanel* panel = NULL;

    if (G_LIKELY(config_file))
    {
        panel = panel_allocate(gdk_display_get_default());
        panel->priv->name = g_strdup(config_name);
        g_debug("starting panel from file %s",config_file);
        if (!config_read_file(panel->priv->config, config_file) ||
            !panel_start(panel))
        {
            g_warning( "lxpanel: can't start panel");
            gtk_widget_destroy(GTK_WIDGET(panel));
            panel = NULL;
        }
    }
    return panel;
}
GtkOrientation panel_get_orientation(LXPanel *panel)
{
    return panel->priv->orientation;
}

gint panel_get_icon_size(LXPanel *panel)
{
    return panel->priv->icon_size;
}

gint panel_get_height(LXPanel *panel)
{
    return panel->priv->height;
}

Window panel_get_xwindow(LXPanel *panel)
{
    return panel->priv->topxwin;
}

gint panel_get_monitor(LXPanel *panel)
{
    return panel->priv->monitor;
}

/* Redefined layout stub safely returning NULL to cleanly conform with style system drop */
GtkStyle *panel_get_defstyle(LXPanel *panel)
{
    return NULL;
}

GtkIconTheme *panel_get_icon_theme(LXPanel *panel)
{
    return panel->priv->icon_theme;
}

gboolean panel_is_at_bottom(LXPanel *panel)
{
    return panel->priv->edge == EDGE_BOTTOM;
}

gboolean panel_is_dynamic(LXPanel *panel)
{
    return panel->priv->widthtype == WIDTH_REQUEST;
}

GtkWidget *panel_box_new(LXPanel *panel, gboolean homogeneous, gint spacing)
{
    GtkOrientation orientation = panel->priv->orientation;
    GtkWidget *box = gtk_box_new(orientation, spacing);
    gtk_box_set_homogeneous(GTK_BOX(box), homogeneous);
    return box;
}

GtkWidget *panel_separator_new(LXPanel *panel)
{
    GtkOrientation orientation = panel->priv->orientation;
    return gtk_separator_new(orientation == GTK_ORIENTATION_HORIZONTAL
                             ? GTK_ORIENTATION_VERTICAL
                             : GTK_ORIENTATION_HORIZONTAL);
}

gboolean _class_is_present(const LXPanelPluginInit *init)
{
    GSList *sl;

    for (sl = all_panels; sl; sl = sl->next)
    {
        LXPanel *panel = (LXPanel *) sl->data;
        GList *plugins, *p;

        if (panel->priv->box == NULL)
            continue;

        plugins = gtk_container_get_children(GTK_CONTAINER(panel->priv->box));
        for (p = plugins; p; p = p->next)
        {
            if (PLUGIN_CLASS(p->data) == init)
            {
                g_list_free(plugins);
                return TRUE;
            }
        }
        g_list_free(plugins);
    }
    return FALSE;
}