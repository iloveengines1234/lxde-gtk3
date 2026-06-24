/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   tree.h for ObConf, the configuration tool for Openbox
   Copyright (c) 2003        Dana Jansens
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#ifndef OBCONF_TREE_H
#define OBCONF_TREE_H

#include <obt/xml.h>

G_BEGIN_DECLS

/**
 * tree_get_node:
 * @path: Forward-slash separated hierarchy path to look up (e.g., "theme/name").
 * @def: Default string payload to apply if node elements must be dynamically generated.
 *
 * Fetches or instantiates a specific node pointer destination inside the XML backend tree structure.
 *
 * Returns: The targeted xmlNodePtr, or NULL if structural generation constraints fail.
 */
xmlNodePtr tree_get_node(const gchar *path, const gchar *def);

/**
 * tree_delete_node:
 * @path: Target node path configuration descriptor to strip away.
 */
void tree_delete_node(const gchar *path);

/* Backend Parsing Element Getters */
gchar   *tree_get_string(const gchar *node, const gchar *def);
gint     tree_get_int(const gchar *node, gint def);
gboolean tree_get_bool(const gchar *node, gboolean def);

/* Backend Parsing Element Mutator Setters */
void tree_set_string(const gchar *node, const gchar *value);
void tree_set_int(const gchar *node, const gint value);
void tree_set_bool(const gchar *node, const gboolean value);

/**
 * tree_apply:
 *
 * Persists the operational runtime XML states down onto the system's rc.xml file
 * and triggers an atomic Inter-Process Communication (IPC) client event loop message 
 * to force Openbox to dynamically reload its environment configuration.
 */
void tree_apply(void);

G_END_DECLS

#endif /* OBCONF_TREE_H */