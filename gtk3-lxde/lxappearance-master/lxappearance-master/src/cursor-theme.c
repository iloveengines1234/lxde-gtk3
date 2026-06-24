/*
 * cursor-theme.c
 
 * Copyright 2010 PCMan <pcman.tw@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * the Free Software Foundation; either version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "cursor-theme.h"
#include "icon-theme.h"
#include "lxappearance.h"

static void update_cursor_demo(void)
{
    GtkListStore* store = gtk_list_store_new(1, GDK_TYPE_PIXBUF);
    GdkCursor* cursor;
    GdkDisplay* display = gdk_display_get_default();
    
    /* Modern GTK3 CSS standard cursor names replacing legacy X11 core cursor shapes */
    const char* names[] = {
        "default",       /* GDK_LEFT_PTR */
        "pointer",       /* GDK_HAND2 */
        "wait",          /* GDK_WATCH */
        "move",          /* GDK_FLEUR */
        "text",          /* GDK_XTERM */
        "w-resize",      /* GDK_LEFT_SIDE */
        "nw-resize",     /* GDK_TOP_LEFT_CORNER */
        "col-resize"     /* GDK_SB_H_DOUBLE_ARROW */
    };
    
    int i;
    for(i = 0; i < (int)G_N_ELEMENTS(names); ++i)
    {
        GtkTreeIter it;
        cursor = gdk_cursor_new_from_name(display, names[i]);
        if (cursor != NULL)
        {
            /* Retrieve target theme representation safely using fallback icon assets */
            GtkIconTheme* icon_theme = gtk_icon_theme_get_default();
            GdkPixbuf* pix = gtk_icon_theme_load_icon(icon_theme, names[i], app.cursor_theme_size, 0, NULL);
            g_object_unref(cursor);
            
            if (pix != NULL)
            {
                gtk_list_store_insert_with_values(store, &it, -1, 0, pix, -1);
                g_object_unref(pix);
            }
        }
    }
    gtk_icon_view_set_model(GTK_ICON_VIEW(app.cursor_demo_view), GTK_TREE_MODEL(store));
    g_object_unref(store);

    /* Update the root window cursor context cleanly via standard modern GTK3 display lookups */
    cursor = gdk_cursor_new_from_name(display, "default");
    if (cursor != NULL)
    {
        GdkWindow* root_win = gdk_get_default_root_window();
        if (root_win != NULL)
        {
            gdk_window_set_cursor(root_win, cursor);
        }
        g_object_unref(cursor);
    }
}

static void on_cursor_theme_sel_changed(GtkTreeSelection* tree_sel, gpointer user_data)
{
    GtkTreeModel* model;
    GtkTreeIter it;
    if(gtk_tree_selection_get_selected(tree_sel, &model, &it))
    {
        IconTheme* theme;
        gtk_tree_model_get(model, &it, 1, &theme, -1);
        if(g_strcmp0(theme->name, app.cursor_theme))
        {
            g_free(app.cursor_theme);
            app.cursor_theme = g_strdup(theme->name);
            g_object_set(gtk_settings_get_default(), "gtk-cursor-theme-name", app.cursor_theme, NULL);

            update_cursor_demo();
            lxappearance_changed();
        }

        gtk_widget_set_sensitive(app.cursor_theme_remove_btn, theme->is_removable);
    }
}

static void on_cursor_theme_size_changed(GtkRange* range, gpointer user_data)
{
    int size = (int)gtk_range_get_value(range);
    if(size != app.cursor_theme_size)
    {
        app.cursor_theme_size = size;
        g_object_set(gtk_settings_get_default(), "gtk-cursor-theme-size", size, NULL);

        update_cursor_demo();
        lxappearance_changed();
    }
}

void cursor_theme_init(GtkBuilder* b)
{
    GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(app.cursor_theme_view));
    g_signal_connect(sel, "changed", G_CALLBACK(on_cursor_theme_sel_changed), NULL);

    app.cursor_size_range = GTK_WIDGET(gtk_builder_get_object(b, "cursor_size"));
    
    /* Set reasonable modern bounds check fallback parameters */
    gtk_range_set_range(GTK_RANGE(app.cursor_size_range), 1, 128); 
    gtk_range_set_value(GTK_RANGE(app.cursor_size_range), app.cursor_theme_size);
    g_signal_connect(app.cursor_size_range, "value-changed", G_CALLBACK(on_cursor_theme_size_changed), NULL);

    /* Set up demo for cursors */
    app.cursor_demo_view = GTK_WIDGET(gtk_builder_get_object(b, "cursor_demo_view"));
    gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(app.cursor_demo_view), 0);
    update_cursor_demo();
}