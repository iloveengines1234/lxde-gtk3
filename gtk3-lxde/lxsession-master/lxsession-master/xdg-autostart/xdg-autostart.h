/*
 *      autostart.h - Handle autostart spec of freedesktop.org
 *
 *      Copyright 2008 PCMan <pcman.tw@gmail.com>
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

#ifndef __XDG_AUTOSTART_H__
#define __XDG_AUTOSTART_H__

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/**
 * xdg_autostart:
 * @session_name: The name identifier of the current desktop environment (e.g., "LXDE").
 *
 * Scans local user configuration contexts and system-wide fallbacks according to 
 * the Freedesktop XDG specification hierarchy rules. Filters out entries via 
 * OnlyShowIn/NotShowIn masks, checks binary executable availability, and launches 
 * valid applications asynchronously.
 */
void xdg_autostart (const char *session_name);

G_END_DECLS

#endif /* __XDG_AUTOSTART_H__ */