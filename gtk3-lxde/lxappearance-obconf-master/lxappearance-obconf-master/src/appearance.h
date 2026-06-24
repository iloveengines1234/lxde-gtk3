/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   appearance.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens
   Copyright (c) 2003        Tim Riley
   Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_APPEARANCE_H
#define OBCONF_APPEARANCE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * appearance_setup_tab:
 *
 * Synchronizes the visual configurations of the user interface tab 
 * by retrieving active Openbox font instances and layout geometry strings 
 * from the underlying database profile tree.
 */
void appearance_setup_tab(void);

G_END_DECLS

#endif /* OBCONF_APPEARANCE_H */