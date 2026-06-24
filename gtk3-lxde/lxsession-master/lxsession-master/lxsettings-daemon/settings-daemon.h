/*
 * settings-daemon.h - LXDE settings daemon
 *
 * Copyright 2009 PCMan <pcman.tw@gmail.com>
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
 * ported by iloveenginesto gtk3 by iloveengines1234
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#ifndef __SETTINGS_DAEMON_H__
#define __SETTINGS_DAEMON_H__

#include <glib.h>

G_BEGIN_DECLS

/* Modern Fix: Forward-declare XEvent instead of embedding heavy X11/Xlib.h directly.
 * This completely prevents namespace pollution in downstream GTK3 interface panels.
 */
typedef union _XEvent XEvent;

/**
 * settings_daemon_start:
 * @kf: A valid, loaded pointer to the system-wide #GKeyFile profile.
 *
 * Establishes standard display pipelines, registers root-window properties, 
 * and connects configuration engines into active memory spaces.
 *
 * Returns: %TRUE on execution success, or %FALSE if a display connection fails.
 */
gboolean settings_daemon_start(GKeyFile *kf);

/**
 * settings_daemon_reload:
 * @kf: An updated or freshly parsed pointer to the desktop's configuration profiles.
 *
 * Forces an active settings daemon to re-verify global system schemas and inject 
 * newly updated configuration keys out to all desktop environments.
 */
void settings_daemon_reload(GKeyFile *kf);

/**
 * settings_manager_selection_clear:
 * @evt: The raw incoming X11 event payload structure passed down by the main loop.
 *
 * Monitored and called by x11_event_dispatch() to process ownership loss if 
 * a competing hardware profile settings manager asserts control over a specific window layer.
 */
void settings_manager_selection_clear(XEvent *evt);

G_END_DECLS

#endif /* __SETTINGS_DAEMON_H__ */