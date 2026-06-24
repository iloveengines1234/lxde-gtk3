#ifndef LXLAUNCHER_WORKING_AREA_H
#define LXLAUNCHER_WORKING_AREA_H

#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * get_working_area:
 * @display: A #GdkDisplay workspace coordinator, or %NULL to automatically 
 * resolve the default system display context.
 * @rect: (out): A non-null #GdkRectangle structure to store the computed 
 * usable screen bounds (subtracting panels, panels, and docks).
 *
 * Calculates the usable monitor workspace boundaries relative to the user's 
 * active cursor location. Natively handles multi-head display monitors, high-DPI 
 * scaling constraints, and structural workarea shifts under both X11 and Wayland backends.
 */
void get_working_area(GdkDisplay *display, GdkRectangle *rect);

G_END_DECLS

#endif /* LXLAUNCHER_WORKING_AREA_H */