/**
 *      Copyright 2008 Fred Chien <cfsghost@gmail.com>
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

#ifndef LXTERMINAL_TAB_H
#define LXTERMINAL_TAB_H

#include <gtk/gtk.h>
#include "lxterminal.h"

/**
 * lxterminal_tab_set_position:
 * @notebook: The GtkNotebook widget context instance.
 * @tabpos: Position ID matching GtkPositionType values.
 *
 * Configures the positioning of tabs around the core terminal area container.
 */
extern void lxterminal_tab_set_position(GtkWidget * notebook, gint tabpos);

/**
 * lxterminal_tab_get_position_id:
 * @position: Text representation identifier of tab alignment (e.g., "top", "bottom").
 *
 * Converts configuration placement string keys into safe structural lookup indexes.
 * Returns: Integer ID representation.
 */
extern gint lxterminal_tab_get_position_id(const gchar * position);

/**
 * lxterminal_tab_label_close_button_clicked:
 * @func: Callback handler triggered on selection.
 * @term: Active terminal layout environment reference state.
 *
 * Links event callbacks to close-action components inside specified terminal tabs.
 */
extern void lxterminal_tab_label_close_button_clicked(GCallback func, Term * term);

/**
 * lxterminal_tab_label_set_text:
 * @tab: Targeted custom container interface structure.
 * @str: Text sequence to render within tab label.
 *
 * Safely mutates string properties inside specified tab widget indicators.
 */
extern void lxterminal_tab_label_set_text(LXTab * tab, const gchar * str);

/**
 * lxterminal_tab_label_set_tooltip_text:
 * @tab: Targeted custom container interface structure.
 * @str: Tooltip instruction content string.
 *
 * Safely applies hover metadata attributes dynamically to the specified tab widget.
 */
extern void lxterminal_tab_label_set_tooltip_text(LXTab * tab, const gchar * str);

/**
 * lxterminal_tab_label_new:
 * @str: Initial string title indicator layout element.
 *
 * Instantiates the low-level UI elements for an custom layout tab wrapper component.
 * Returns: A newly allocated LXTab reference context instance.
 */
extern LXTab * lxterminal_tab_label_new(const gchar * str);

/**
 * lxterminal_tab_label_free:
 * @tab: LXTab instantiation component context to destroy.
 *
 * Deallocates runtime operational footprint references for structural tabs safely.
 */
extern void lxterminal_tab_label_free(LXTab * tab);

#endif /* LXTERMINAL_TAB_H */