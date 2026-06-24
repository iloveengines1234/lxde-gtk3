/*
 * Copyright (C) 2009 Marty Jack <martyj19@comcast.net>
 *               2014 Vladimír Pýcha <vpycha@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef ICON_GRID_OLD_H
#define ICON_GRID_OLD_H

#include <gtk/gtk.h>
#include "panel.h"

G_BEGIN_DECLS

struct _icon_grid_element;
struct _icon_grid;

/* Representative of an icon grid element. 
 * This is a widget to be packed into a rectangular grid whose size adapts to conditions. */
typedef struct _icon_grid_element {
    struct _icon_grid_element *flink;       /* Forward link */
    struct _icon_grid         *ig;          /* Back pointer to IconGrid */
    GtkWidget                 *widget;      /* Customer's widget */
    gboolean                   visible;     /* True if widget is visible */
} IconGridElement;

/* Representative of an icon grid. 
 * This is a manager that packs widgets into a rectangular grid whose size adapts to conditions. */
typedef struct _icon_grid {
    IconGridElement *child_list;               /* List of icon grid elements */
    Panel           *panel;                    /* Back pointer to panel */
    GtkWidget       *container;                /* Container widget */
    GtkOrientation   orientation;              /* Desired orientation */
    gint             child_width;              /* Desired child width */
    gint             child_height;             /* Desired child height */
    gint             spacing;                  /* Desired spacing between grid elements */
    gint             border;                   /* Desired border around grid elements */
    gint             target_dimension;         /* Desired dimension perpendicular to orientation */
    gboolean         constrain_width;          /* True if width should be constrained by allocated space */
    gboolean         actual_dimension;         /* True if container has been allocated space */
    gboolean         children_changed;         /* True if icon grid element list changed */
    GtkWidget       *widget;                   /* Layout widget we use for packing (GtkFixed) */
    int              rows;                     /* Computed layout rows */
    int              columns;                  /* Computed layout columns */
    int              container_width;          /* Container's allocated width */
    int              container_height;         /* Container's allocated height */
    int              constrained_child_width;  /* Child width constrained by allocation */
} IconGrid;

/* Public API Prototyping */

IconGrid * icon_grid_new(Panel          *panel, 
                         GtkWidget      *container,
                         GtkOrientation  orientation, 
                         gint            child_width, 
                         gint            child_height, 
                         gint            spacing, 
                         gint            border, 
                         gint            target_dimension);

void icon_grid_set_constrain_width(IconGrid *ig, gboolean constrain_width);

void icon_grid_set_geometry(IconGrid      *ig,
                            GtkOrientation orientation, 
                            gint           child_width, 
                            gint           child_height, 
                            gint           spacing, 
                            gint           border, 
                            gint           target_dimension);

void icon_grid_add(IconGrid *ig, GtkWidget *child, gboolean visible);

void icon_grid_remove(IconGrid *ig, GtkWidget *child);

gint icon_grid_get_child_position(IconGrid *ig, GtkWidget *child);

void icon_grid_reorder_child(IconGrid *ig, GtkWidget *child, gint position);

void icon_grid_set_visible(IconGrid *ig, GtkWidget *child, gboolean visible);

void icon_grid_free(IconGrid *ig);

G_END_DECLS

#endif /* ICON_GRID_OLD_H */