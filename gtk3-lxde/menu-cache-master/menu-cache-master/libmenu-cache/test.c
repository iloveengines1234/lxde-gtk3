/*
 *      test.c
 *      
 *      Copyright 2008 PCMan <pcman.tw@gmail.com>
 *      
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *      
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *      
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <gtk/gtk.h>
#include "menu-cache.h"

static void on_menu_item(GtkMenuItem* mi, MenuCacheItem* item)
{
    g_debug("Exec = %s", menu_cache_app_get_exec(MENU_CACHE_APP(item)));
    g_debug("Terminal = %d", menu_cache_app_get_use_terminal(MENU_CACHE_APP(item)));
}

static GtkWidget* create_item(MenuCacheItem* item)
{
    GtkWidget* mi;
    if (menu_cache_item_get_type(item) == MENU_CACHE_TYPE_SEP)
    {
        mi = gtk_separator_menu_item_new();
    }
    else
    {
        GtkWidget* box;
        GtkWidget* label;
        GtkWidget* img;
        const char* icon_name;

        mi = gtk_menu_item_new();
        gtk_widget_set_tooltip_text(mi, menu_cache_item_get_comment(item));

        /* Use a horizontal box to pack image and label inside modern GtkMenuItem */
        box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        
        icon_name = menu_cache_item_get_icon(item);
        if (!icon_name || *icon_name == '\0')
            icon_name = "application-x-executable"; /* Safe fallback icon */

        img = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_MENU);
        label = gtk_label_new(menu_cache_item_get_name(item));

        gtk_box_pack_start(GTK_BOX(box), img, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(mi), box);

        if (menu_cache_item_get_type(item) == MENU_CACHE_TYPE_APP)
        {
            g_object_set_data_full(G_OBJECT(mi), "mcitem", menu_cache_item_ref(item), (GDestroyNotify)menu_cache_item_unref);
            g_signal_connect(mi, "activate", G_CALLBACK(on_menu_item), item);
        }
    }
    gtk_widget_show_all(mi);
    return mi;
}

static GtkWidget* create_menu(MenuCacheDir* dir, GtkWidget* parent)
{
    GSList* l;
    GtkWidget* menu = gtk_menu_new();
    GtkWidget* mi;

    if (parent)
    {
        mi = create_item(MENU_CACHE_ITEM(dir));
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), menu);
        gtk_menu_shell_append(GTK_MENU_SHELL(parent), mi);
        gtk_widget_show(mi);
    }

    /* Used updated API call path list to match header adaptations */
    for (l = menu_cache_dir_list_children(dir); l; l = l->next)
    {
        MenuCacheItem* item = MENU_CACHE_ITEM(l->data);

        if (menu_cache_item_get_type(item) == MENU_CACHE_TYPE_DIR)
        {
            create_menu(MENU_CACHE_DIR(item), menu);
        }
        else
        {
            mi = create_item(item);
            gtk_widget_show(mi);
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        }
    }
    return menu;
}

static void on_clicked(GtkButton* btn, GtkWidget* pop)
{
    /* Refactored legacy popup calculation to the modern preferred GTK 3.24 implementation */
    gtk_menu_popup_at_widget(GTK_MENU(pop), 
                             GTK_WIDGET(btn), 
                             GDK_GRAVITY_SOUTH_WEST, 
                             GDK_GRAVITY_NORTH_WEST, 
                             NULL);
}

int main(int argc, char** argv)
{
    GtkWidget* win;
    GtkWidget* btn;
    GtkWidget* pop;
    MenuCache* menu_cache;
    MenuCacheDir* menu;
    const char* menu_path;

    gtk_init(&argc, &argv);

    menu_path = (argc > 1) ? argv[1] : "/etc/xdg/menus/applications.menu";
    
    /* Kept alignment to updated cache instantiation frameworks */
    menu_cache = menu_cache_lookup_sync(menu_path);
    if (!menu_cache)
    {
        g_critical("Failed to build or allocate reference to menu cache context.");
        return 1;
    }

    menu = menu_cache_dup_root_dir(menu_cache);
    if (!menu)
    {
        g_critical("Failed to retrieve target root directory payload from cache structure.");
        menu_cache_unref(menu_cache);
        return 1;
    }

    pop = create_menu(menu, NULL);

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win), "MenuCache Test");
    gtk_window_set_default_size(GTK_WINDOW(win), 250, 100);

    btn = gtk_button_new_with_label(menu_cache_item_get_name(MENU_CACHE_ITEM(menu)));
    gtk_widget_set_tooltip_text(btn, menu_cache_item_get_comment(MENU_CACHE_ITEM(menu)));
    
    gtk_container_add(GTK_CONTAINER(win), btn);
    
    g_signal_connect(btn, "clicked", G_CALLBACK(on_clicked), pop);
    g_signal_connect(win, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(win);

    /* Release locally duplicated root node tracking references cleanly */
    menu_cache_item_unref(MENU_CACHE_ITEM(menu));
    menu_cache_unref(menu_cache);

    gtk_main();
    return 0;
}