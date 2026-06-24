/*
 * Guifications - The end all, be all, toaster popup plugin
 * Copyright (C) 2003-2004 Gary Kramlich
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/*
  2026.06.05 Modified by iloveengines1234
  This piece of code detecting working area is got from Guifications, a plug-in for Gaim, and modernized with GTK3.24.52
*/

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

void get_working_area(GdkScreen *screen, GdkRectangle *rect);

static gboolean gf_display_get_workarea(GdkScreen *g_screen, GdkRectangle *rect)
{
    Atom xa_desktops, xa_current, xa_workarea, xa_type;
    Display *x_display;
    Window x_root;
    guint32 current = 0;
    gulong *workareas, len, fill;
    guchar *data = NULL;
    gint format;

    GdkDisplay *g_display;
    Screen *x_screen;

    /* Get the GDK display */
    g_display = gdk_display_get_default();
    if (!g_display)
        return FALSE;

    /* Get the X display from the GDK display */
    x_display = gdk_x11_display_get_xdisplay(g_display);
    if (!x_display)
        return FALSE;

    /* Get the X screen from the GDK screen */
    x_screen = gdk_x11_screen_get_xscreen(g_screen);
    if (!x_screen)
        return FALSE;

    /* Get the root window from the screen */
    x_root = XRootWindowOfScreen(x_screen);

    /* Find the _NET_NUMBER_OF_DESKTOPS atom */
    xa_desktops = XInternAtom(x_display, "_NET_NUMBER_OF_DESKTOPS", True);
    if (xa_desktops == None)
        return FALSE;

    /* Get the number of desktops */
    if (XGetWindowProperty(x_display, x_root, xa_desktops, 0, 1, False,
                           XA_CARDINAL, &xa_type, &format, &len, &fill,
                           &data) != Success) {
        return FALSE;
    }

    if (!data)
        return FALSE;

    /* Just verify it fetched properly, then release immediately */
    XFree(data);
    data = NULL;

    /* Find the _NET_CURRENT_DESKTOP atom */
    xa_current = XInternAtom(x_display, "_NET_CURRENT_DESKTOP", True);
    if (xa_current == None)
        return FALSE;

    /* Get the current desktop index */
    if (XGetWindowProperty(x_display, x_root, xa_current, 0, 1, False,
                           XA_CARDINAL, &xa_type, &format, &len, &fill,
                           &data) != Success) {
        return FALSE;
    }

    if (!data)
        return FALSE;

    current = *(guint32 *)data;
    XFree(data);
    data = NULL;

    /* Find the _NET_WORKAREA atom */
    xa_workarea = XInternAtom(x_display, "_NET_WORKAREA", True);
    if (xa_workarea == None)
        return FALSE;

    /* Fetch workareas for all desktops */
    if (XGetWindowProperty(x_display, x_root, xa_workarea, 0, (glong)(4 * 32),
                           False, AnyPropertyType, &xa_type, &format, &len,
                           &fill, &data) != Success) {
        return FALSE;
    }

    /* Guard and safely free data if properties are invalid */
    if (xa_type == None || format == 0 || fill || (len % 4)) {
        if (data)
            XFree(data);
        return FALSE;
    }

    /* Extract geometry data for current desktop workspace */
    workareas = (gulong *)data;

    rect->x      = (gint)workareas[current * 4];
    rect->y      = (gint)workareas[current * 4 + 1];
    rect->width  = (gint)workareas[current * 4 + 2];
    rect->height = (gint)workareas[current * 4 + 3];

    XFree(data);
    return TRUE;
}

void get_working_area(GdkScreen *screen, GdkRectangle *rect)
{
    if (!gf_display_get_workarea(screen, rect)) {
        /* Fallback if EWMH properties are missing or call fails */
        rect->x = 0;
        rect->y = 0;
        rect->width = gdk_screen_width();
        rect->height = gdk_screen_height();
    }
}