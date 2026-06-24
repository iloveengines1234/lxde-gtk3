/**
 *      Copyright 2008 Fred Chien <cfsghost@gmail.com>
 *      Copyright (c) 2010 LxDE Developers, see the file AUTHORS for details.
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

#ifndef LXTERMINAL_UNIXSOCKET_H
#define LXTERMINAL_UNIXSOCKET_H

#include <gtk/gtk.h>
#include "lxterminal.h"

/**
 * lxterminal_socket_initialize:
 * @lxtermwin: Core structural window frame reference mapping context.
 * @profile: Active runtime profile key configuration or NULL for the default profile.
 * @argc: Number of strings passed inside the argv array argument vector.
 * @argv: Array of command line options string pointers forwarded from the process.
 *
 * Checks for a pre-existing primary process instance via Unix sockets. 
 * If found, forwards arguments to the server and signals that this child process should exit.
 * If not found, initializes a local server interface endpoint context instead.
 *
 * Returns: TRUE if this instance should keep executing as the host runner, FALSE to close.
 */
extern gboolean lxterminal_socket_initialize(LXTermWindow * lxtermwin, const char * profile, gint argc, gchar ** argv);

#endif /* LXTERMINAL_UNIXSOCKET_H */