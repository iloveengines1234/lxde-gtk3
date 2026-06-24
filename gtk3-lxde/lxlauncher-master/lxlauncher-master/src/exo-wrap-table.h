/* $Id: exo-wrap-table.h 18995 2005-12-05 16:46:54Z benny $ */
/*-
 * Copyright (c) 2000 Ramiro Estrugo <ramiro@eazel.com>
 * Copyright (c) 2005 Benedikt Meurer <benny@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* 2008.05.07 modified by Hong Jen Yee
 * Remove other libexo dependencies for use in LXDE. */


#ifndef EXO_WRAP_TABLE_H
#define EXO_WRAP_TABLE_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef struct _ExoWrapTable ExoWrapTable;

/**
 * exo_wrap_table_new:
 * @homogeneous: Whether all child allocation cells share identical grid sizing.
 *
 * Allocates a new grid-reflowing container widget instance layout.
 *
 * Returns: (transfer full): A newly allocated #GtkWidget wrapper container.
 */
GtkWidget *exo_wrap_table_new(gboolean homogeneous) G_GNUC_MALLOC;

/* --- API Grid Spacing Management Attributes --- */

guint exo_wrap_table_get_col_spacing(const ExoWrapTable *table);
void  exo_wrap_table_set_col_spacing(ExoWrapTable       *table,
                                     guint               col_spacing);

guint exo_wrap_table_get_row_spacing(const ExoWrapTable *table);
void  exo_wrap_table_set_row_spacing(ExoWrapTable       *table,
                                     guint               row_spacing);

gboolean exo_wrap_table_get_homogeneous(const ExoWrapTable *table);
void     exo_wrap_table_set_homogeneous(ExoWrapTable       *table,
                                        gboolean            homogeneous);

G_END_DECLS

#endif /* !EXO_WRAP_TABLE_H */