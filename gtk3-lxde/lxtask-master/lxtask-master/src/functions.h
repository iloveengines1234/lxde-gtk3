/* $Id: functions.h 3940 2008-02-10 22:48:45Z nebulon $
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 * Updated by iloveengines1234 to support modern GTK 3 compilation environments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <glib.h>
#include "types.h"

G_BEGIN_DECLS

/* --- Core System Metrics Parsing Engine --- */

/**
 * refresh_task_list:
 *
 * Loops through the active kernel process tree, updates internal data collections,
 * and refreshes the tree store UI elements.
 *
 * Returns: TRUE if processing succeeded, FALSE if a critical access error occurred.
 */
gboolean refresh_task_list(void);

/**
 * get_cpu_usage:
 * @sys_stat: Pointer to the persistent system status delta tracking structure.
 *
 * Calculates the total system-wide CPU utilization percentage across jiffy intervals.
 *
 * Returns: A percentage value scaled between 0.0 and 100.0.
 */
gdouble get_cpu_usage(system_status *sys_stat);


/* --- Persistent User Configuration Subsystem --- */

/**
 * load_config:
 *
 * Read settings from the local profile file layout, falling back to safe 
 * application defaults if parsing fails.
 */
void load_config(void);

/**
 * save_config:
 *
 * Serializes active preferences, window boundaries, and view flags 
 * back to the application configuration file.
 */
void save_config(void);

/**
 * reset_config_defaults:
 * * Flushes active configuration flags back to runtime defaults. Useful for 
 * error recovery blocks when processing corrupted application config layers.
 */
void reset_config_defaults(void);


/* --- Platform-Specific Architecture Hooks --- */
#if defined(__linux__) || defined(__linux) || defined(linux)
#include "xfce-taskmanager-linux.h"
#endif

G_END_DECLS

#endif /* FUNCTIONS_H */