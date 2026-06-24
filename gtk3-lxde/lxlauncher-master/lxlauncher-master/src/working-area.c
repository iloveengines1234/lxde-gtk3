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
  2026.06.04 Modified by iloveengines1234
  This piece of code detecting working area is got from Guifications, a plug-in for Gaim. Also I didn't realize at the time of writing
  this that I did port this to gtk3.24.52, so I updated the code to use gtk3 APIs and removed some deprecated calls. The original code is licensed under GPLv2+.
*/

#include "working-area.h"

/**
 * get_working_area:
 * @display: Target #GdkDisplay workspace coordinator, or %NULL to auto-resolve.
 * @area: (out): Pointer to a #GdkRectangle structure where results are saved.
 *
 * Populates @area with the usable monitor dimensions surrounding the user's cursor.
 * Accounts automatically for display panel reservations and high-DPI scaling.
 */
void get_working_area(GdkDisplay *display, GdkRectangle *area)
{
    GdkSeat *seat;
    GdkDevice *pointer;
    GdkMonitor *monitor = NULL;
    gint x = 0, y = 0;

    /* Safe hardware baseline boundaries default initialization fallback */
    if (area) {
        area->x = 0;
        area->y = 0;
        area->width = 1024;
        area->height = 768;
    } else {
        return;
    }

    /* Automatically fall back to the default display backend if none provided */
    if (G_UNLIKELY(!display)) {
        display = gdk_display_get_default();
    }

    if (G_UNLIKELY(!display)) {
        return;
    }

    /* --- Wayland & X11 Clean Abstraction Layer --- */
    seat = gdk_display_get_default_seat(display);
    if (G_LIKELY(seat)) {
        pointer = gdk_seat_get_pointer(seat);
        if (G_LIKELY(pointer)) {
            /* * Natively samples the cursor location directly out of the central 
             * device coordinator map, without relying on unstable legacy Root Windows.
             */
            gdk_device_get_position(pointer, NULL, &x, &y);
            monitor = gdk_display_get_monitor_at_point(display, x, y);
        }
    }

    /* Fallback to primary workspace index monitor if device positioning failed */
    if (G_UNLIKELY(!monitor)) {
        monitor = gdk_display_get_primary_monitor(display);
        if (!monitor) {
            monitor = gdk_display_get_monitor(display, 0);
        }
    }

    /* Extract and commit structural desktop boundary information */
    if (G_LIKELY(monitor)) {
        /* * Native GTK3 utility function. Pulls accurate geometry profiles, 
         * automatically stripping panel bars, docks, and workspace margins.
         */
        gdk_monitor_get_workarea(monitor, area);
    }
}