/* pager.c -- pager module of lxpanel project
 *
 * Copyright (C) 2009 Dongxu Li <song6song@sourceforge.net>
 *               2012 Julien Lavergne <gilir@ubuntu.com>
 *               2014 Andriy Grytsenko <andrej@rep.kiev.ua>
 *               2018 number Zero <numzero@users.sf.net>
 *               2025 Ingo Brückl
 *
 * This file is part of lxpanel.
 *
 * lxpanel is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * lxpanel is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sawfish; see the file COPYING.   If not, write to
 * the Free Software Foundation, 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 */

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <libfm/fm-gtk.h>

#include <glib/gi18n.h>
#ifndef WNCK_I_KNOW_THIS_IS_UNSTABLE
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#endif
#include <libwnck/libwnck.h>

#include "misc.h"
#include "plugin.h"

typedef struct
{
    WnckPager *pager;
    gint border;
} PagerData;

/* command to configure desktop, it will be set by .config callback */
static const char *configure_command = NULL;

static void on_realize(GtkWidget *p, LXPanel *panel)
{
    PagerData *d = lxpanel_plugin_get_data(p);
    int rows, r, h = panel_get_height(panel) - 2 * d->border;

    /* set geometry */
    wnck_pager_set_orientation(d->pager, panel_get_orientation(panel));
    if (panel_get_orientation(panel) == GTK_ORIENTATION_VERTICAL)
        h *= ((gfloat) gdk_screen_height() / (gfloat) gdk_screen_width());
    rows = h / (panel_get_icon_size(panel) * 2) + 1; /* min */
    r = (h - 2) / panel_get_icon_size(panel); /* max */
    /* g_debug("pager for height %d and icon size %d: %d to %d",panel_get_height(panel),panel_get_icon_size(panel),r,rows); */
    rows = MAX(rows, r);
    wnck_pager_set_n_rows(d->pager, rows);
}

static void on_size_allocate(GtkWidget *p, GdkRectangle *allocation, LXPanel *panel)
{
    /* g_debug("pager: on_size_allocate(): %dx%d", allocation->width, allocation->height); */
    on_realize(p, panel);
}

static gboolean on_scroll_event(GtkWidget * p, GdkEventScroll * ev, LXPanel *panel)
{
    int desknum = get_net_current_desktop();
    int desks = get_net_number_of_desktops();
    Screen *xscreen = GDK_SCREEN_XSCREEN(gtk_widget_get_screen(p));

    switch (ev->direction) {
        case GDK_SCROLL_DOWN:
            desknum++;
            break;
        case GDK_SCROLL_UP:
            desknum--;
            break;
        default:
            return FALSE;
    }

    if (desknum < 0 || desknum >= desks)
        return TRUE;

    Xclimsgx(xscreen, RootWindowOfScreen(xscreen), a_NET_CURRENT_DESKTOP, desknum, 0, 0, 0, 0);
    return TRUE;
}

#if GTK_CHECK_VERSION(3, 0, 0)
static void on_style_updated(GtkWidget *p, LXPanel *panel)
{
    PagerData *d = lxpanel_plugin_get_data(p);
    GtkStyleContext *context = gtk_widget_get_style_context(GTK_WIDGET(d->pager));
    GtkCssProvider *provider = gtk_css_provider_new();
    GdkRGBA color;
    gchar *color_str;
    gchar *bg_css;

    gtk_style_context_add_class(context, "wnck-pager");
    /* Provide a fallback color for the highlighted workspace based on the current theme */
    if (!gtk_style_context_lookup_color(context, "theme_selected_bg_color", &color))
        gdk_rgba_parse(&color, "#3584e4");
    color_str = gdk_rgba_to_string(&color);
    bg_css = g_strconcat(".wnck-pager:selected {\n"
                         "	background-color:", color_str, ";\n"
                         "}", NULL);
    gtk_css_provider_load_from_data(provider, bg_css, -1, NULL);
    g_free(bg_css);
    g_free(color_str);

    gtk_style_context_add_provider(context,
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_FALLBACK);

    g_object_unref(provider);
}
#endif

static GtkWidget *pager_constructor(LXPanel *panel, config_setting_t *settings)
{
    GtkWidget *p, *w;
    PageData *d;
    int border = 1; 

    /* Dynamically read panel layout border settings if present,
     * fallback to 1 pixel if undefined in the active profile. */
    if (settings)
    {
        /* Attempt to look up a standardized border size or style padding token */
        if (!config_setting_lookup_int(settings, "border", &border))
            border = 1; 
    }
    else if (panel)
    {
        /* Fall back to standard panel spacing profiles if settings blocks are unmapped */
        border = (lxpanel_get_padding(panel) > 0) ? 1 : 0;
    }

    /* Modern Libwnck-3.0 initialization pattern */
    w = wnck_pager_new();
    g_return_val_if_fail(w != NULL, NULL);

    d = g_new0(PageData, 1);
    d->pager = WNCK_PAGER(w);
    d->border = border;

    p = gtk_event_box_new();
    gtk_widget_set_has_window(p, FALSE);
    lxpanel_plugin_set_data(p, d, g_free);

    /* Attach standard operation callback signals */
    g_signal_connect(p, "realize", G_CALLBACK(on_realize), panel);
    g_signal_connect(p, "size-allocate", G_CALLBACK(on_size_allocate), panel);
    g_signal_connect(p, "scroll-event", G_CALLBACK(on_scroll_event), panel);
    g_signal_connect(p, "style-updated", G_CALLBACK(on_style_updated), panel);

    wnck_pager_set_display_mode(d->pager, WNCK_PAGER_DISPLAY_CONTENT);
    gtk_widget_show(w);

    /* Add an additional layout wrapper container to isolate border spacing rules safely */
    w = gtk_event_box_new();
    gtk_widget_set_has_window(w, FALSE);

    /* Migrated deprecated container borders to modern GTK3 Margin constraints */
    gtk_widget_set_margin_start(GTK_WIDGET(d->pager), border);
    gtk_widget_set_margin_end(GTK_WIDGET(d->pager), border);
    gtk_widget_set_margin_top(GTK_WIDGET(d->pager), border);
    gtk_widget_set_margin_bottom(GTK_WIDGET(d->pager), border);

    gtk_container_add(GTK_CONTAINER(w), GTK_WIDGET(d->pager));
    gtk_widget_show(w);
    gtk_container_add(GTK_CONTAINER(p), w);

    return p;
}

/* this is a modified version of patch from Lubuntu */
static GtkWidget *pager_configure(LXPanel *panel, GtkWidget *instance)
{
    /* Use explicit UI matching definitions if external WM controls exist */
    if (configure_command && configure_command[0] != '\0')
    {
        fm_launch_command_simple(NULL, NULL, G_APP_INFO_CREATE_NONE, configure_command, NULL);
    }
    else
    {
        /* Provide fallback dialog execution warning overlay natively */
        fm_show_error(GTK_WINDOW(gtk_widget_get_toplevel(instance)), NULL,
                      _("Sorry, there was no window manager configuration program found."));
    }
    return NULL; /* no configuration dialog of lxpanel available */
}

static void pager_menu_callback(GtkWidget *widget, gpointer data)
{
    gtk_widget_set_sensitive(widget, FALSE);
}

static gboolean pager_update_context_menu(GtkWidget *plugin, GtkMenu *menu)
{
    GdkScreen *screen = gtk_widget_get_screen(plugin);
    const char *wm_name = gdk_x11_screen_get_window_manager_name(screen);
    gchar *path = NULL;
    const char *display_name = NULL;

    /* Clear legacy global configuration pointers */
    configure_command = NULL;

    /* REWRITE RESOLVED FIXME: Expanded support to dynamically map multiple active window managers */
    if (g_strcmp0(wm_name, "Openbox") == 0)
    {
        if ((path = g_find_program_in_path("obconf")))
        {
            configure_command = "obconf --tab 6";
            display_name = "Obconf";
        }
    }
    else if (g_strcmp0(wm_name, "compiz") == 0)
    {
        if ((path = g_find_program_in_path("ccsm")))
        {
            configure_command = "ccsm";
            display_name = "CCSM";
        }
        else if ((path = g_find_program_in_path("simple-ccsm")))
        {
            configure_command = "simple-ccsm";
            display_name = "Simple CCSM";
        }
    }
    else if (g_strcmp0(wm_name, "Xfwm4") == 0)
    {
        if ((path = g_find_program_in_path("xfwm4-settings")))
        {
            configure_command = "xfwm4-settings";
            display_name = "Xfwm4 Settings";
        }
    }
    else if (g_strcmp0(wm_name, "IceWM") == 0)
    {
        if ((path = g_find_program_in_path("icecc")))
        {
            configure_command = "icecc";
            display_name = "IceWM Control Center";
        }
    }
    else if (g_ascii_strcasecmp(wm_name, "KWin") == 0)
    {
        if ((path = g_find_program_in_path("systemsettings")))
        {
            /* Launch directly into the KDE Virtual Desktops configuration module */
            configure_command = "systemsettings kcm_virtualdesktops";
            display_name = "KDE System Settings";
        }
    }

    /* Set up our search payload context block */
    PagerMenuSearchCtx ctx;
    ctx.command = configure_command;
    ctx.wm_display_name = display_name;
    ctx.found = FALSE;

    /* Traverse menu elements to find and apply dynamic properties modifications */
    if (menu)
    {
        gtk_container_foreach(GTK_CONTAINER(menu), update_pager_settings_menu_item, &ctx);
    }

    /* String paths are now safely cleaned from memory */
    if (path)
    {
        g_free(path);
    }

    return FALSE;
}

static void pager_panel_configuration_changed(LXPanel *panel, GtkWidget *p)
{
    on_realize(p, panel);
}

static LXPanelPluginInit wnck_pager = {
    .name = N_("Desktop Pager"),
    .description = N_("Simple pager plugin"),

    .superseded = TRUE,
    .new_instance = pager_constructor,
    .config = pager_configure,
    .update_context_menu = pager_update_context_menu,
    .reconfigure = pager_panel_configuration_changed
};

static void pager_wnck_init(void)
{
    lxpanel_register_plugin_type("wnckpager", &wnck_pager);
}

LXPanelPluginInit lxpanel_static_plugin_pager = {
    .name = N_("Desktop Pager"),
    .description = N_("Simple pager plugin"),

    .init = pager_wnck_init,
    .new_instance = pager_constructor,
    .config = pager_configure,
    .update_context_menu = pager_update_context_menu,
    .reconfigure = pager_panel_configuration_changed
};
