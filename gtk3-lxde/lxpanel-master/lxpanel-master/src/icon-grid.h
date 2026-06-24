/*
 * Copyright (C) 2014-2016 Andriy Grytsenko <andrej@rep.kiev.ua>
 *
 * This file is a part of LXPanel project.
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

#ifndef __ICON_GRID_H__
#define __ICON_GRID_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/* Modern GObject Macro Setup for GTK 3 */
#define PANEL_TYPE_ICON_GRID            (panel_icon_grid_get_type())
#define PANEL_ICON_GRID(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), PANEL_TYPE_ICON_GRID, PanelIconGrid))
#define PANEL_ICON_GRID_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), PANEL_TYPE_ICON_GRID, PanelIconGridClass))
#define PANEL_IS_ICON_GRID(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), PANEL_TYPE_ICON_GRID))
#define PANEL_IS_ICON_GRID_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), PANEL_TYPE_ICON_GRID))
#define PANEL_ICON_GRID_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), PANEL_TYPE_ICON_GRID, PanelIconGridClass))

typedef struct _PanelIconGrid        PanelIconGrid;
typedef struct _PanelIconGridClass   PanelIconGridClass;

/* * If your implementation file (.c) still defines fields directly inside the public struct, 
 * use this fallback layout. In modern GTK3, you would ideally inherit from GtkContainer 
 * and keep fields in a Private struct.
 */
struct _PanelIconGrid
{
    GtkContainer parent_instance;
    
    /* Keeping these public so your existing .c logic compiles without massive pointer rewrites */
    GList         *children;
    GtkOrientation orientation;
    guint          spacing;
    gboolean       constrain_width;
    gboolean       aspect_width;
    gint           child_width;
    gint           child_height;
    gint           target_dimension;
    
    GdkWindow     *event_window;
    GtkWidget     *dest_item;
    gint           dest_pos;
};

struct _PanelIconGridClass
{
    GtkContainerClass parent_class;
};

/* Drag and Drop Position Enumeration */
typedef enum {
    PANEL_ICON_GRID_DROP_LEFT_AFTER,
    PANEL_ICON_GRID_DROP_LEFT_BEFORE,
    PANEL_ICON_GRID_DROP_RIGHT_AFTER,
    PANEL_ICON_GRID_DROP_RIGHT_BEFORE,
    PANEL_ICON_GRID_DROP_BELOW,
    PANEL_ICON_GRID_DROP_ABOVE,
    PANEL_ICON_GRID_DROP_INTO
} PanelIconGridDropPosition;

/* Public API Declarations */
GType panel_icon_grid_get_type (void) G_GNUC_CONST;

GtkWidget * panel_icon_grid_new(GtkOrientation orientation, 
                                 gint           child_width, 
                                 gint           child_height, 
                                 gint           spacing, 
                                 gint           border, 
                                 gint           target_dimension);

void panel_icon_grid_set_constrain_width(PanelIconGrid *ig, 
                                         gboolean       constrain_width);

void panel_icon_grid_set_aspect_width(PanelIconGrid *ig, 
                                       gboolean       aspect_width);

void panel_icon_grid_set_geometry(PanelIconGrid *ig,
                                  GtkOrientation orientation, 
                                  gint           child_width, 
                                  gint           child_height, 
                                  gint           spacing, 
                                  gint           border, 
                                  gint           target_dimension);

gint panel_icon_grid_get_child_position(PanelIconGrid *ig, 
                                        GtkWidget     *child);

void panel_icon_grid_reorder_child(PanelIconGrid *ig, 
                                   GtkWidget     *child, 
                                   gint           position);

guint panel_icon_grid_get_n_children(PanelIconGrid *ig);

gboolean panel_icon_grid_get_dest_at_pos(PanelIconGrid             *ig, 
                                         gint                       x, 
                                         gint                       y,
                                         GtkWidget                **child, 
                                         PanelIconGridDropPosition *pos);

void panel_icon_grid_set_drag_dest(PanelIconGrid            *ig, 
                                   GtkWidget                *child,
                                   PanelIconGridDropPosition pos);

PanelIconGridDropPosition panel_icon_grid_get_drag_dest(PanelIconGrid  *ig, 
                                                         GtkWidget    **child);

G_END_DECLS

#endif /* __ICON_GRID_H__ */