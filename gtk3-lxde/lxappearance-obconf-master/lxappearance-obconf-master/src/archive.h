/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   archive.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens
   Copyright (c) 2003        Tim Riley
   Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_ARCHIVE_H
#define OBCONF_ARCHIVE_H

#include <glib.h>

G_BEGIN_DECLS

/**
 * archive_install:
 * @path: The absolute filesystem path pointing to an Openbox Theme archive (.obt)
 *
 * Extracts and registers an Openbox Theme into the localized target profile sandbox folder.
 *
 * Returns: (transfer full): The text string identifying the theme name inside the archive, 
 * or NULL if extraction fails. The caller assumes memory ownership.
 */
gchar *archive_install(const gchar *path);

/**
 * archive_create:
 * @path: The source folder pathway target holding a standard Openbox runtime layout schema.
 *
 * Compresses an Openbox theme profile hierarchy into a shared distribution archive (.obt format).
 */
void archive_create(const gchar *path);

G_END_DECLS

#endif /* OBCONF_ARCHIVE_H */