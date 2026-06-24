/*
 * Copyright (C) 2014-2016 Andriy Grytsenko <andrej@rep.kiev.ua>
 *               2024 Ingo Brückl
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

#ifndef __GTK_COMPAT_H__
#define __GTK_COMPAT_H__ 1

#include <gtk/gtk.h>
#include <gdk/gdk.h>

/* * Modern GTK3 Compatibility Shims
 * Ancient GTK2 fallback checks (< 2.24) have been dropped.
 */

/* GTK3 completely removed dialog separators, making this flag obsolete */
#ifndef GTK_DIALOG_NO_SEPARATOR
#  define GTK_DIALOG_NO_SEPARATOR 0
#endif

/* * If your legacy code uses gtk_vbox_new or gtk_hbox_new in places we haven't touched yet,
 * these macros will automatically safely map them to standard GTK3 GtkBoxes.
 */
#ifndef gtk_hbox_new
#  define gtk_hbox_new(homogeneous, spacing) \
     gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing)
#endif

#ifndef gtk_vbox_new
#  define gtk_vbox_new(homogeneous, spacing) \
     gtk_box_new(GTK_ORIENTATION_VERTICAL, spacing)
#endif

#endif /* __GTK_COMPAT_H__ */