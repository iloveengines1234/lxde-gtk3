/*
 *      glib-compat.h
 *
 *      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *
 *      This file is a part of the Libfm library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * glib-compat.h
 *
 * This file is kept intentionally empty to satisfy legacy includes.
 * All historical version fallback macros have been purged.
 * Natively targets GLib baselines >= 2.88.1.
 */

#ifndef __GLIB_COMPAT_H__
#define __GLIB_COMPAT_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* * All legacy workarounds for G_DEFINE_INTERFACE, g_list_free_full, 
 * g_slist_free_full, and g_slist_copy_deep have been completely removed.
 * They are provided natively by standard modern GLib implementations.
 */

G_END_DECLS

#endif /* __GLIB_COMPAT_H__ */