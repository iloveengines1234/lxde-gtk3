/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   theme.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens
   Copyright (c) 2003        Tim Riley
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_THEME_H
#define OBCONF_THEME_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * theme_setup_tab:
 *
 * Allocates, populates, and wires up the GtkListStore column components
 * needed to display the interface's theme sidebar switcher view.
 */
void theme_setup_tab(void);

/**
 * theme_load_all:
 *
 * Scans local personal directory trees (~/.themes), systemic data layout directories,
 * and standard paths to re-populate the selection list with valid Openbox profiles.
 */
void theme_load_all(void);

/**
 * theme_install:
 * @path: The absolute filesystem path specifying a target Openbox theme archive (.obt).
 *
 * Extracts and deploys an incoming theme bundle file into the user's local space.
 */
void theme_install(const gchar *path);

G_END_DECLS

#endif /* OBCONF_THEME_H */