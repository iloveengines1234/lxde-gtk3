/***************************************************************************
 *   Copyright (C) 2007 by PCMan (Hong Jen Yee) <pcman.tw@gmail.com>       *
 *   Copyright (C) 2023-2024 Ingo Brückl                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "image-view.h"
#include <math.h>

static void image_view_finalize(GObject *iv);
static void image_view_clear( ImageView* iv );
static void calc_image_area( ImageView* iv );

static void paint( ImageView* iv, GdkRectangle* invalid_rect, GdkInterpType type, cairo_t* cr );
static void image_view_paint( ImageView* iv, cairo_t* cr );

static void on_get_preferred_width( GtkWidget* widget, gint* minimal_width, gint* natural_width );
static void on_get_preferred_height( GtkWidget* widget, gint* minimal_height, gint* natural_height );
static void on_size_allocate( GtkWidget* widget, GtkAllocation *allocation );
static gboolean on_draw_event(GtkWidget* widget, cairo_t* cr);

// In GTK3, custom drawing widgets must inherit from GTK_TYPE_WIDGET directly
G_DEFINE_TYPE( ImageView, image_view, GTK_TYPE_WIDGET );

void image_view_init( ImageView* iv )
{
    iv->pix = NULL;
    iv->scale = 1.0;
    iv->interp_type = GDK_INTERP_BILINEAR;
    iv->idle_handler = 0;

    gtk_widget_set_can_focus((GtkWidget*)iv, TRUE );
    gtk_widget_set_app_paintable((GtkWidget*)iv, TRUE );
}

void image_view_class_init( ImageViewClass* klass )
{
    GObjectClass *obj_class = ( GObjectClass * ) klass;
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS ( klass );

    obj_class->finalize = image_view_finalize;

    widget_class->get_preferred_width = on_get_preferred_width;
    widget_class->get_preferred_height = on_get_preferred_height;
    widget_class->draw = on_draw_event;
    widget_class->size_allocate = on_size_allocate;
}

void image_view_finalize(GObject *iv)
{
    image_view_clear( (ImageView*)iv );
    G_OBJECT_CLASS(image_view_parent_class)->finalize(iv);
}

GtkWidget* image_view_new()
{
    return (GtkWidget*)g_object_new ( IMAGE_VIEW_TYPE, NULL );
}

void image_view_set_adjustments( ImageView* iv, GtkAdjustment* h, GtkAdjustment* v )
{
    if( iv->hadj != h )
    {
        if( iv->hadj )
            g_object_unref( iv->hadj );
        if( G_LIKELY(h) )
            iv->hadj = (GtkAdjustment*)g_object_ref_sink( h );
        else
            iv->hadj = NULL;
    }
    if( iv->vadj != v )
    {
        if( iv->vadj )
            g_object_unref( iv->vadj );
        if( G_LIKELY(v) )
            iv->vadj = (GtkAdjustment*)g_object_ref_sink( v );
        else
            iv->vadj = NULL;
    }
}

void on_get_preferred_width( GtkWidget* widget, gint* minimal_width, gint* natural_width )
{
    ImageView* iv = (ImageView*)widget;
    *minimal_width = *natural_width = iv->img_area.width;
}

void on_get_preferred_height( GtkWidget* widget, gint* minimal_height, gint* natural_height )
{
    ImageView* iv = (ImageView*)widget;
    *minimal_height = *natural_height = iv->img_area.height;
}

void on_size_allocate( GtkWidget* widget, GtkAllocation *allocation )
{
    GTK_WIDGET_CLASS(image_view_parent_class)->size_allocate( widget, allocation );
    ImageView* iv = (ImageView*)widget;

    iv->allocation = *allocation;
    calc_image_area( iv );
}

static cairo_region_t *
eel_cairo_get_clip_region (cairo_t *cr)
{
    cairo_rectangle_list_t *list;
    cairo_region_t *region;
    int i;

    list = cairo_copy_clip_rectangle_list (cr);
    if (list->status == CAIRO_STATUS_CLIP_NOT_REPRESENTABLE) {
        cairo_rectangle_int_t clip_rect;
        cairo_rectangle_list_destroy (list);

        if (!gdk_cairo_get_clip_rectangle (cr, &clip_rect))
                return NULL;
        return cairo_region_create_rectangle (&clip_rect);
    }

    region = cairo_region_create ();
    for (i = list->num_rectangles - 1; i >= 0; --i) {
        cairo_rectangle_t *rect = &list->rectangles[i];
        cairo_rectangle_int_t clip_rect;

        clip_rect.x = floor (rect->x);
        clip_rect.y = floor (rect->y);
        clip_rect.width = ceil (rect->x + rect->width) - clip_rect.x;
        clip_rect.height = ceil (rect->y + rect->height) - clip_rect.y;

        if (cairo_region_union_rectangle (region, &clip_rect) != CAIRO_STATUS_SUCCESS) {
                cairo_region_destroy (region);
                region = NULL;
                break;
        }
    }

    cairo_rectangle_list_destroy (list);
    return region;
}

static gboolean on_draw_event( GtkWidget* widget, cairo_t *cr )
{
    ImageView* iv = (ImageView*)widget;
    if ( gtk_widget_get_mapped(widget) )
        image_view_paint( iv, cr );
    return FALSE;
}

void image_view_paint( ImageView* iv, cairo_t *cr )
{
    if( iv->pix )
    {
        cairo_region_t * region = eel_cairo_get_clip_region(cr);
        if (!region) return;
        
        int n_rects = cairo_region_num_rectangles(region);
        int i;
        for( i = 0; i < n_rects; ++i )
        {
            cairo_rectangle_int_t rectangle;
            cairo_region_get_rectangle(region, i, &rectangle);
            paint( iv, &rectangle, iv->interp_type, cr );
        }

        cairo_region_destroy (region);
    }
}

void image_view_clear( ImageView* iv )
{
    if( iv->idle_handler )
    {
        g_source_remove( iv->idle_handler );
        iv->idle_handler = 0;
    }

    if( iv->pix )
    {
        g_object_unref( iv->pix );
        iv->pix = NULL;
        calc_image_area( iv );
    }
}

void image_view_set_pixbuf( ImageView* iv, GdkPixbuf* pixbuf )
{
    if( pixbuf != iv->pix )
    {
        image_view_clear( iv );
        if( G_LIKELY(pixbuf) )
            iv->pix = (GdkPixbuf*)g_object_ref( pixbuf );
        calc_image_area( iv );
        gtk_widget_queue_resize( (GtkWidget*)iv );
    }
}

void image_view_set_scale( ImageView* iv, gdouble new_scale, GdkInterpType type )
{
    if( new_scale == iv->scale )
        return;

    GtkWidget *widget = GTK_WIDGET(iv);
    gint xPos = 0, yPos = 0;
    
    // Modern GTK3 pointer extraction mechanism replacing gtk_widget_get_pointer
    GdkWindow *window = gtk_widget_get_window(widget);
    if (window) {
        GdkDisplay *display = gdk_window_get_display(window);
        GdkSeat *seat = gdk_display_get_default_seat(display);
        GdkDevice *pointer = gdk_seat_get_pointer(seat);
        gdk_window_get_device_position(window, pointer, &xPos, &yPos, NULL);
    }

    gdouble hadj_value = gtk_adjustment_get_value(iv->hadj);
    gdouble vadj_value = gtk_adjustment_get_value(iv->vadj);

    if (xPos < hadj_value || xPos > hadj_value + gtk_adjustment_get_page_size(iv->hadj) ||
        yPos < vadj_value || yPos > vadj_value + gtk_adjustment_get_page_size(iv->vadj))
    {
        xPos = iv->img_area.width / 2;
        yPos = iv->img_area.height / 2;
    }

    gdouble oldRelativePositionX = (gdouble) xPos / iv->img_area.width;
    gdouble oldRelativePositionY = (gdouble) yPos / iv->img_area.height;
    gdouble visibleAreaX = xPos - hadj_value;
    gdouble visibleAreaY = yPos - vadj_value;

    iv->scale = new_scale;
    if( iv->pix )
    {
        calc_image_area( iv );

        hadj_value = oldRelativePositionX * iv->img_area.width - visibleAreaX;
        vadj_value = oldRelativePositionY * iv->img_area.height - visibleAreaY;

        gtk_adjustment_set_value(iv->hadj, hadj_value > 0 ? hadj_value : 0);
        gtk_adjustment_set_value(iv->vadj, vadj_value > 0 ? vadj_value : 0);

        gtk_widget_queue_resize( widget );
    }
}

void calc_image_area( ImageView* iv )
{
    if( G_LIKELY( iv->pix ) )
    {
        GtkAllocation allocation = iv->allocation;
        iv->img_area.width = (int)floor( ((gdouble)gdk_pixbuf_get_width( iv->pix )) * iv->scale + 0.5 );
        iv->img_area.height = (int)floor( ((gdouble)gdk_pixbuf_get_height( iv->pix )) * iv->scale + 0.5 );

        int x_offset = 0, y_offset = 0;
        if( iv->img_area.width < allocation.width )
            x_offset = (int)floor( ((gdouble)(allocation.width - iv->img_area.width)) / 2 + 0.5);
        if( iv->img_area.height < allocation.height )
            y_offset = (int)floor( ((gdouble)(allocation.height - iv->img_area.height)) / 2 + 0.5);

        iv->img_area.x = x_offset;
        iv->img_area.y = y_offset;
    }
    else
    {
        iv->img_area.x = iv->img_area.y = iv->img_area.width = iv->img_area.height = 0;
    }
}

void paint( ImageView* iv, GdkRectangle* invalid_rect, GdkInterpType type, cairo_t* cr )
{
    GdkRectangle rect;
    if( ! gdk_rectangle_intersect( invalid_rect, &iv->img_area, &rect ) )
        return;

    int dest_x = rect.x;
    int dest_y = rect.y;

    rect.x -= iv->img_area.x;
    rect.y -= iv->img_area.y;

    int src_x = (int)floor( ((gdouble)rect.x) / iv->scale + 0.5 );
    int src_y = (int)floor( ((gdouble)rect.y) / iv->scale + 0.5 );
    int src_w = (int)floor( ((gdouble)rect.width) / iv->scale + 0.5 );
    int src_h = (int)floor( ((gdouble)rect.height) / iv->scale + 0.5 );
    
    if( src_y > gdk_pixbuf_get_height( iv->pix ) )
        src_y = gdk_pixbuf_get_height( iv->pix );
    if( src_x + src_w > gdk_pixbuf_get_width( iv->pix ) )
        src_w = gdk_pixbuf_get_width( iv->pix ) - src_x;
    if( src_y + src_h > gdk_pixbuf_get_height( iv->pix ) )
        src_h = gdk_pixbuf_get_height( iv->pix ) - src_y;

    GdkPixbuf* src_pix = NULL;
    GdkPixbuf* scaled_pix = NULL;

    if ((src_w > 0) && (src_h > 0))
    {
        src_pix = gdk_pixbuf_new_subpixbuf( iv->pix, src_x, src_y, src_w, src_h );
        scaled_pix = gdk_pixbuf_scale_simple( src_pix, rect.width, rect.height, type );
        g_object_unref( src_pix );
        src_pix = scaled_pix;
    }

    if( G_LIKELY(src_pix) )
    {
        gdk_cairo_set_source_pixbuf (cr, src_pix, dest_x, dest_y);
        cairo_paint (cr);
        g_object_unref( src_pix );
    }
}

void image_view_get_size( ImageView* iv, int* w, int* h )
{
    if( G_LIKELY(w) )
        *w = iv->img_area.width;
    if( G_LIKELY(h) )
        *h = iv->img_area.height;
}