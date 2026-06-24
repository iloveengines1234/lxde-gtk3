/* $Id: exo-wrap-table.c 22884 2006-08-26 12:40:43Z benny $ */
/*-
 * Copyright (c) 2000      Ramiro Estrugo <ramiro@eazel.com>
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
 * 2025      Ingo Brückl
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "exo-wrap-table.h"

/* Private data structure */
struct _ExoWrapTablePrivate
{
  GList *children;
  guint  col_spacing;
  guint  row_spacing;
  guint  homogeneous : 1;
  gint   num_cols;
};

G_DEFINE_TYPE_WITH_PRIVATE (ExoWrapTable, exo_wrap_table, GTK_TYPE_CONTAINER)

#define EXO_WRAP_TABLE_GET_PRIVATE(obj) (exo_wrap_table_get_instance_private (EXO_WRAP_TABLE (obj)))

/* Forward declarations */
static void exo_wrap_table_get_preferred_width         (GtkWidget *widget,
                                                        gint      *minimal_width,
                                                        gint      *natural_width);
static void exo_wrap_table_get_preferred_height        (GtkWidget *widget,
                                                        gint      *minimal_height,
                                                        gint      *natural_height);
static void exo_wrap_table_get_preferred_height_for_width (GtkWidget *widget,
                                                           gint       width,
                                                           gint      *minimal_height,
                                                           gint      *natural_height);
static void exo_wrap_table_size_allocate               (GtkWidget          *widget,
                                                        GtkAllocation      *allocation);
static void exo_wrap_table_add                         (GtkContainer       *container,
                                                        GtkWidget          *widget);
static void exo_wrap_table_remove                      (GtkContainer       *container,
                                                        GtkWidget          *widget);
static void exo_wrap_table_forall                      (GtkContainer       *container,
                                                        gboolean            include_internals,
                                                        GtkCallback         callback,
                                                        gpointer            callback_data);

static gint exo_wrap_table_get_max_child_size          (const ExoWrapTable *table,
                                                        gint               *max_width_return,
                                                        gint               *max_height_return);
static gint exo_wrap_table_get_num_fitting             (gint                available,
                                                        gint                spacing,
                                                        gint                max_child_size);

/* Helper: compute max child requisition among visible children and return count */
static gint
exo_wrap_table_get_max_child_size (const ExoWrapTable *table,
                                   gint               *max_width_return,
                                   gint               *max_height_return)
{
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (table);
  GList *list;
  GtkRequisition req;
  gint max_w = 0, max_h = 0;
  gint count = 0;

  for (list = priv->children; list != NULL; list = list->next)
    {
      GtkWidget *child = GTK_WIDGET (list->data);
      if (!gtk_widget_get_visible (child))
        continue;
      gtk_widget_get_preferred_size (child, &req, NULL);
      if (req.width > max_w) max_w = req.width;
      if (req.height > max_h) max_h = req.height;
      count++;
    }

  if (max_width_return) *max_width_return = max_w;
  if (max_height_return) *max_height_return = max_h;

  return count;
}

/* Compute how many children of size max_child_size fit into available
 * space given spacing. Always return at least 1.
 */
static gint
exo_wrap_table_get_num_fitting (gint available, gint spacing, gint max_child_size)
{
  if (max_child_size <= 0) return 1;
  gint num = (available + spacing) / (max_child_size + spacing);
  return (num > 0) ? num : 1;
}

/* get_preferred_width: report minimal and natural width as max child width.
 * The height-for-width override will compute heights for a given width.
 */
static void
exo_wrap_table_get_preferred_width (GtkWidget *widget,
                                    gint      *minimal_width,
                                    gint      *natural_width)
{
  gint max_w = 0, max_h = 0;
  guint n = exo_wrap_table_get_max_child_size (EXO_WRAP_TABLE (widget), &max_w, &max_h);

  if (n == 0)
    {
      if (minimal_width) *minimal_width = 0;
      if (natural_width) *natural_width = 0;
      return;
    }

  if (minimal_width) *minimal_width = max_w;
  if (natural_width) *natural_width = max_w;
}

/* get_preferred_height: delegate to height-for-width using natural width */
static void
exo_wrap_table_get_preferred_height (GtkWidget *widget,
                                     gint      *minimal_height,
                                     gint      *natural_height)
{
  gint min_w, nat_w;
  gtk_widget_get_preferred_width (widget, &min_w, &nat_w);
  exo_wrap_table_get_preferred_height_for_width (widget, nat_w, minimal_height, natural_height);
}

/* FIXED: Implemented height-for-width calculation logic for adaptive reflow containers
 *
 * Note: This implementation assumes that children are laid out uniformly using
 * the maximum child requisition (max width and max height among visible
 * children). That is, children are treated as if they have identical size
 * equal to the maximum observed child size when computing rows/columns.
 */
static void
exo_wrap_table_get_preferred_height_for_width (GtkWidget *widget,
                                               gint       width,
                                               gint      *minimal_height,
                                               gint      *natural_height)
{
  ExoWrapTable *table = EXO_WRAP_TABLE (widget);
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (table);
  gint max_w = 0, max_h = 0;
  guint n = exo_wrap_table_get_max_child_size (table, &max_w, &max_h);

  if (n == 0)
    {
      if (minimal_height) *minimal_height = 0;
      if (natural_height) *natural_height = 0;
      return;
    }

  gint cols = exo_wrap_table_get_num_fitting (width, priv->col_spacing, max_w);
  if (cols < 1) cols = 1;

  gint rows = n / cols;
  if ((n % cols) > 0) rows++;

  gint height = (rows * max_h) + ((rows - 1) * priv->row_spacing);

  if (minimal_height) *minimal_height = height;
  if (natural_height) *natural_height = height;
}

/* size_allocate: allocate children using uniform cell size computed from max child requisition */
static void
exo_wrap_table_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  ExoWrapTable *table = EXO_WRAP_TABLE (widget);
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (table);
  gint max_w = 0, max_h = 0;
  guint n = exo_wrap_table_get_max_child_size (table, &max_w, &max_h);
  GList *list;

  gtk_widget_set_allocation (widget, allocation);

  if (n == 0) return;

  gint cols = exo_wrap_table_get_num_fitting (allocation->width, priv->col_spacing, max_w);
  if (cols < 1) cols = 1;

  gint cur_col = 0;
  gint cur_row = 0;

  for (list = priv->children; list != NULL; list = list->next)
    {
      GtkWidget *child = GTK_WIDGET (list->data);
      if (!gtk_widget_get_visible (child))
        continue;

      GtkAllocation child_alloc;
      child_alloc.x = allocation->x + cur_col * (max_w + priv->col_spacing);
      child_alloc.y = allocation->y + cur_row * (max_h + priv->row_spacing);
      child_alloc.width = MAX(0, max_w);
      child_alloc.height = MAX(0, max_h);

      gtk_widget_size_allocate (child, &child_alloc);

      cur_col++;
      if (cur_col >= cols)
        {
          cur_col = 0;
          cur_row++;
        }
    }
}

/* container API: add/remove/forall */
static void
exo_wrap_table_add (GtkContainer *container, GtkWidget *widget)
{
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (container);

  if (!GTK_IS_WIDGET (widget))
    return;

  if (g_list_find (priv->children, widget))
    return;

  if (gtk_widget_get_parent (widget) && gtk_widget_get_parent (widget) != GTK_WIDGET (container))
    gtk_widget_unparent (widget);

  priv->children = g_list_append (priv->children, widget);
  gtk_widget_set_parent (widget, GTK_WIDGET (container));
  gtk_widget_queue_resize (GTK_WIDGET (container));
}

static void
exo_wrap_table_remove (GtkContainer *container, GtkWidget *widget)
{
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (container);
  GList *l = g_list_find (priv->children, widget);
  if (l)
    {
      gtk_widget_unparent (widget);
      priv->children = g_list_delete_link (priv->children, l);
      gtk_widget_queue_resize (GTK_WIDGET (container));
    }
}

static void
exo_wrap_table_forall (GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data)
{
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (container);
  GList *list;

  for (list = priv->children; list != NULL; list = list->next)
    {
      GtkWidget *child = GTK_WIDGET (list->data);
      if (gtk_widget_get_visible (child) || include_internals)
        (*callback) (child, callback_data);
    }
}

/* widget init */
static void
exo_wrap_table_init (ExoWrapTable *table)
{
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (table);
  priv->children = NULL;
  priv->col_spacing = 0;
  priv->row_spacing = 0;
  priv->num_cols = 0;
  priv->homogeneous = FALSE;
  gtk_widget_set_has_window (GTK_WIDGET (table), FALSE);
}

/* class init */
static void
exo_wrap_table_class_init (ExoWrapTableClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

  widget_class->get_preferred_width = exo_wrap_table_get_preferred_width;
  widget_class->get_preferred_height = exo_wrap_table_get_preferred_height;
  widget_class->get_preferred_height_for_width = exo_wrap_table_get_preferred_height_for_width;
  widget_class->size_allocate = exo_wrap_table_size_allocate;

  container_class->add = exo_wrap_table_add;
  container_class->remove = exo_wrap_table_remove;
  container_class->forall = exo_wrap_table_forall;
}

/* public constructor */
GtkWidget *
exo_wrap_table_new (gboolean homogeneous)
{
  ExoWrapTable *table = g_object_new (EXO_TYPE_WRAP_TABLE, NULL);
  ExoWrapTablePrivate *priv = EXO_WRAP_TABLE_GET_PRIVATE (table);
  priv->homogeneous = homogeneous;
  return GTK_WIDGET (table);
}