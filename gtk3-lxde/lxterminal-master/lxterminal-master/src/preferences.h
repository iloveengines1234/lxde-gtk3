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

#ifndef LXTERMINAL_PREFERENCES_H
#define LXTERMINAL_PREFERENCES_H

#include <gtk/gtk.h>
#include "lxterminal.h"

/**
 * terminal_preferences_dialog:
 * @action: The GtkAction that triggered the dialog selection.
 * @terminal: The current active LXTerminal window context instance.
 *
 * Initializes, builds, and executes the preferences modal configuration window.
 */
extern void terminal_preferences_dialog(GtkAction * action, LXTerminal * terminal);

/**
 * terminal_tab_get_position_id:
 * @position: The textual representation of the tab layout position (e.g., "top", "bottom").
 *
 * Converts a string-based preference designation into its corresponding GtkNotebook position index.
 * Returns: An integer ID mapping safely inside the preferences UI store indices.
 */
extern gint terminal_tab_get_position_id(const gchar * position);

#endif /* LXTERMINAL_PREFERENCES_H */