/* Metacity fixed tooltip routine */

/* 
 * Copyright (C) 2001 Havoc Pennington
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, 
 * MA 02110-1301 USA.
 */

#include "fixedtip.h"

static GtkWidget *tip = NULL;
static GtkWidget *label = NULL;
static int screen_width = 0;
static int screen_height = 0;

static gboolean
button_press_handler (GtkWidget *widget,
                      GdkEvent  *event,
                      void      *data)
{
  fixed_tip_hide ();
  return FALSE;
}

/* Replaced expose_handler with modern GTK3 draw handler */
static gboolean
draw_handler (GtkWidget *widget,
              cairo_t   *cr,
              gpointer   user_data)
{
  GtkStyleContext *context;

  context = gtk_widget_get_style_context (widget);

  /* Render a standard tooltip flat background box using the CSS theme */
  gtk_render_background (context, cr, 0, 0,
                         gtk_widget_get_allocated_width (widget),
                         gtk_widget_get_allocated_height (widget));
                         
  gtk_render_frame (context, cr, 0, 0,
                    gtk_widget_get_allocated_width (widget),
                    gtk_widget_get_allocated_height (widget));

  return FALSE;
}

void
fixed_tip_show (int screen_number,
                int root_x, int root_y,
                gboolean strut_is_vertical,
                int strut,
                const char *markup_text)
{
  int w, h;
  
  if (tip == NULL)
    {      
      tip = gtk_window_new (GTK_WINDOW_POPUP);
      
      /* Modern multihead handling via GdkScreen updates */
      GdkDisplay *display = gdk_display_get_default ();
      GdkScreen *gdk_screen = gdk_display_get_screen (display, screen_number);
      
      gtk_window_set_screen (GTK_WINDOW (tip), gdk_screen);
      
      /* Fallback fallback if screen monitors layout gets complicated */
      screen_width = gdk_screen_get_width (gdk_screen);
      screen_height = gdk_screen_get_height (gdk_screen);
      
      gtk_widget_set_app_paintable (tip, TRUE);
      gtk_window_set_resizable (GTK_WINDOW (tip), FALSE);
      
      /* Style classes are used instead of explicit names in GTK3 */
      GtkStyleContext *context = gtk_widget_get_style_context (tip);
      gtk_style_context_add_class (context, GTK_STYLE_CLASS_TOOLTIP);
      
      gtk_container_set_border_width (GTK_CONTAINER (tip), 4);

      /* Use draw signal instead of expose_event */
      g_signal_connect (G_OBJECT (tip), 
                        "draw",
                        G_CALLBACK (draw_handler),
                        NULL);
   
      gtk_widget_add_events (tip, GDK_BUTTON_PRESS_MASK);
      
      g_signal_connect (G_OBJECT (tip), 
                        "button_press_event",
                        G_CALLBACK (button_press_handler),
                        NULL);
      
      label = gtk_label_new (NULL);
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      
      /* Replaced GtkMisc alignments with modern widget property accessors */
      gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
      gtk_widget_set_valign (label, GTK_ALIGN_CENTER);
      gtk_widget_show (label);
      
      gtk_container_add (GTK_CONTAINER (tip), label);

      g_signal_connect (G_OBJECT (tip),
                        "destroy",
                        G_CALLBACK (gtk_widget_destroyed),
                        &tip);
    }

  gtk_label_set_markup (GTK_LABEL (label), markup_text);
  
  /* Make sure geometry calculation respects window resizing sizes */
  gtk_window_get_size (GTK_WINDOW (tip), &w, &h);

  /* pad between panel and message window */
#define PAD 5
  
  if (strut_is_vertical)
    {
      if (strut > root_x)
        root_x = strut + PAD;
      else
        root_x = strut - w - PAD;

      root_y -= h / 2;
    }
  else
    {
      if (strut > root_y)
        root_y = strut + PAD;
      else
        root_y = strut - h - PAD;

      root_x -= w / 2;
    }

  /* Push onscreen safely */
  if ((root_x + w) > screen_width)
    root_x -= (root_x + w) - screen_width;

  if ((root_y + h) > screen_height)
    root_y -= (root_y + h) - screen_height;
  
  gtk_window_move (GTK_WINDOW (tip), root_x, root_y);
  gtk_widget_show (tip);
}

void
fixed_tip_hide (void)
{
  if (tip)
    {
      gtk_widget_destroy (tip);
      tip = NULL;
    }
}