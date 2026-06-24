/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   main.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003        Dana Jansens
   Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_MAIN_H
#define OBCONF_MAIN_H

#include <obrender/render.h>
#include <obrender/instance.h>
#include <obt/xml.h>
#include <obt/paths.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Global variables shared across plug-in files */
extern GtkBuilder  *builder;
extern RrInstance  *rrinst;
extern GtkWidget   *mainwin;
extern gchar       *obc_config_file;
extern ObtPaths    *paths;
extern ObtXmlInst  *xml_i;

/**
 * get_widget:
 * @s: The structural UI element identifier token string.
 *
 * Safe wrapper to isolate and cast GtkBuilder object lookup references securely.
 */
#define get_widget(s) GTK_WIDGET(gtk_builder_get_object(builder, (s)))

/**
 * obconf_error:
 * @msg: The localized error descriptive string payload.
 * @modal: Set to TRUE if the layout requires asynchronous modality block constraints.
 *
 * Spawns a standard non-blocking error interface notification panel layer.
 */
void obconf_error(gchar *msg, gboolean modal);

G_END_DECLS

#endif /* OBCONF_MAIN_H */