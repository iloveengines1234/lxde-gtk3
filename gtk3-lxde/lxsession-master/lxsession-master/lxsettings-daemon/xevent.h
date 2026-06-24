/*
 * xevent.h
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
 * Ported by iloveengines1234 to gtk3
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#ifndef __XEVENT_H__
#define __XEVENT_H__

#include <glib.h>

G_BEGIN_DECLS

/* Modern Fix: Forward-declare Display to prevent leaking X11 macro pollution 
 * (like Success, None, True) into downstream GTK 3 interface namespaces. */
typedef struct _XDisplay Display;

/* The shared application-wide pointer to the active X display connection stream */
extern Display *dpy;

/**
 * LXS_CMD:
 * @LXS_RELOAD: Requests an immediate configuration reload from disk profiles.
 * @LXS_EXIT: Triggers the session framework teardown loop.
 * @LXS_LAST_CMD: Boundary token for validation checks.
 *
 * Internal control message codes exchanged via root-window ClientMessages.
 */
typedef enum {
    LXS_RELOAD,
    LXS_EXIT,
    LXS_LAST_CMD
} LXS_CMD;

/**
 * xevent_init:
 *
 * Establishes connection to the primary X display server, registers private 
 * window atoms, and binds the connection descriptor directly into the GLib GSource loop.
 *
 * Returns: %TRUE if the connection succeeds, otherwise %FALSE.
 */
gboolean xevent_init(void);

/**
 * single_instance_check:
 *
 * Asserts an atomic selection grab over the display workspace to ensure 
 * only one instance of the session management pipeline is running.
 *
 * Returns: %TRUE if this process successfully claimed the environment lock.
 */
gboolean single_instance_check(void);

/**
 * xevent_finalize:
 *
 * Teardown procedure to cleanly detach the active main loop event source, 
 * ungrab server layers, and drop the primary display connection interface safely.
 */
void xevent_finalize(void);

/**
 * send_internal_command:
 * @cmd: The target operational command token value from the #LXS_CMD schema.
 *
 * Injects a type-safe ClientMessage pipeline event payload directly back onto the 
 * root window context layer to coordinate asynchronous session requests.
 */
void send_internal_command(LXS_CMD cmd);

G_END_DECLS

#endif /* __XEVENT_H__ */