/*
 *      gtk-compat.h
 *
 *      Copyright 2011 - 2012 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

/*
 * gtk-compat.h
 *
 * This file is kept intentionally empty to satisfy legacy include traces.
 * Historical GTK 2/3 translation macros have been completely removed.
 * Natively targets GTK baselines >= 3.24.52.
 */

#ifndef __GTK_COMPAT_H__
#define __GTK_COMPAT_H__

#include <gtk/gtk.h>
#include "glib-compat.h"

G_BEGIN_DECLS

/* * All legacy compatibility workarounds for key defines, gdk_window manipulation,
 * opaque structure extraction (GdkDragContext), ATK versioning, and selection data 
 * extraction have been purged. Standard native GTK 3 methods are resolved directly.
 */

G_END_DECLS

#endif /* __GTK_COMPAT_H__ */