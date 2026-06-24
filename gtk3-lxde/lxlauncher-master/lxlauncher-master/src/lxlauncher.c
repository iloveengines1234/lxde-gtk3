/*
 * lxlauncher.c - Open source replace for EeePC Asus Launcher
 *
 * Copyright 2008 PCMan <pcman.tw@gmail.com>
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * Updated for clean, standard GTK3.24.52 compilation by iloveengines1234
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <gdk/gdkx.h>
#include <X11/XID.h>

#include "exo-wrap-table.h"
#include "misc.h"
#include "working-area.h"

#define BUTTON_SIZE_FALLBACK 110
#define IMG_SIZE_FALLBACK 48

static GtkIconSize icon_size;
static int button_size;
static int img_size;

static GtkWidget* main_window;
static GtkWidget* notebook;
static GKeyFile* key_file;

typedef struct _AppButton AppButton;
struct _AppButton
{
    char* exec;
    char* name;
    GtkWidget* widget;
};

static void on_app_btn_clicked(GtkWidget* btn, AppButton* app)
{
    GdkScreen* screen = gtk_widget_get_screen(btn);
    GError* err = NULL;
    if (!lxlauncher_execute_app(screen, NULL, app->exec, app->name, NULL, FALSE, &err))
    {
        GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                GTK_DIALOG_MODAL,
                                                GTK_MESSAGE_ERROR,
                                                GTK_BUTTONS_OK,
                                                _("Failed to execute %s\n%s"),
                                                app->name, err->message);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        g_error_free(err);
    }
}

static void on_menu_selection_done(GtkWidget* menu, gpointer user_data)
{
    gtk_widget_destroy(menu);
}

static void on_menu_edit(GtkWidget* item, const char* file_path)
{
    char* cmd = g_strdup_printf("lxshortcut -e \"%s\"", file_path);
    GError* err = NULL;
    if (!g_spawn_command_line_async(cmd, &err))
    {
        g_warning("Failed to run lxshortcut: %s", err->message);
        g_error_free(err);
    }
    g_free(cmd);
}

static gboolean on_app_btn_press_event(GtkWidget* btn, GdkEventButton* evt, AppButton* app)
{
    if (evt->button == 3)
    {
        char* tmp = g_find_program_in_path("lxshortcut");
        if (tmp)
        {
            GtkWidget* p = gtk_menu_new();
            GtkWidget* item = gtk_menu_item_new_with_mnemonic(_("_Edit Item"));
            const char* file_path = g_object_get_data(G_OBJECT(btn), "desktop-file-path");

            g_signal_connect(item, "activate", G_CALLBACK(on_menu_edit), (gpointer)file_path);
            gtk_menu_shell_append(GTK_MENU_SHELL(p), item);
            gtk_widget_show_all(p);

            g_signal_connect(p, "selection-done", G_CALLBACK(on_menu_selection_done), NULL);
            gtk_menu_popup_at_pointer(GTK_MENU(p), (GdkEvent*)evt);
            
            g_free(tmp);
            return TRUE;
        }
    }
    return FALSE;
}

static void free_app_btn(GtkWidget* widget, AppButton* app)
{
    g_free(app->exec);
    g_free(app->name);
    g_free(app);
}

static GtkWidget* add_btn(GtkWidget* table, const char* text, GdkPixbuf* icon, const char* tip)
{
    GtkWidget *btn, *box, *img, *label;
    gint req_width, req_height;
    GtkSettings *settings = gtk_widget_get_settings(GTK_WIDGET(main_window));
    gboolean enable_key = 0;
    g_object_get(settings, "lxlauncher-enable-key", &enable_key, NULL);
    GtkStyleContext *context;
    GtkCssProvider *css_provider;
    int btn_border;

    btn = gtk_button_new();
    gtk_widget_set_size_request(btn, button_size, -1);
    if (!enable_key)
    {
        gtk_widget_set_can_focus(btn, FALSE);
        gtk_widget_set_can_default(GTK_WIDGET(btn), FALSE);
    }
    img = gtk_image_new_from_pixbuf(icon);

    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_pack_start(GTK_BOX(box), img, FALSE, TRUE, 2);

    label = gtk_label_new(text);
    gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
    gtk_label_set_xalign(GTK_LABEL(label), 0.5);
    gtk_label_set_yalign(GTK_LABEL(label), 0.0);
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(btn), box);

    gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
    gtk_widget_set_size_request(btn, button_size, button_size);
    gtk_widget_show_all(btn);

    gtk_container_add(GTK_CONTAINER(table), btn);

    gtk_widget_set_tooltip_text(btn, tip);

    context = gtk_widget_get_style_context(btn);
    GtkBorder padding, border;
    gtk_style_context_get_padding(context, GTK_STATE_FLAG_NORMAL, &padding);
    gtk_style_context_get_border(context, GTK_STATE_FLAG_NORMAL, &border);
    
    /* FIXED: Removed deprecated calls to gtk_container_get_border_width */
    btn_border = padding.left + padding.right + border.left + border.right;

    gtk_widget_get_preferred_width(label, &req_width, &req_height);

    if (req_width > (button_size - btn_border))
    {
        gtk_widget_set_size_request(label, button_size - btn_border, -1);
        gtk_label_set_line_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
    }

    gtk_widget_set_app_paintable(btn, TRUE);
    
    /* FIXED: Modernized background transparency overriding mechanism using GtkCssProvider styling context injection */
    css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css_provider, "button { background: transparent; background-color: transparent; border-color: transparent; }", -1, NULL);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css_provider);

    return btn;
}

static void load_desktop_entry(const char* file_path, GtkWidget* table)
{
    GKeyFile* kf = g_key_file_new();
    if (g_key_file_load_from_file(kf, file_path, G_KEY_FILE_NONE, NULL))
    {
        char* type = g_key_file_get_locale_string(kf, "Desktop Entry", "Type", NULL, NULL);
        gboolean nodisplay = g_key_file_get_boolean(kf, "Desktop Entry", "NoDisplay", NULL);

        if (nodisplay || (type && strcmp(type, "Application") != 0))
        {
            g_free(type);
            g_key_file_free(kf);
            return;
        }
        g_free(type);

        char* name = g_key_file_get_locale_string(kf, "Desktop Entry", "Name", NULL, NULL);
        char* exec = g_key_file_get_locale_string(kf, "Desktop Entry", "Exec", NULL, NULL);
        char* icon_name = g_key_file_get_locale_string(kf, "Desktop Entry", "Icon", NULL, NULL);
        char* comment = g_key_file_get_locale_string(kf, "Desktop Entry", "Comment", NULL, NULL);

        if (name && exec)
        {
            GdkPixbuf* icon = lxlauncher_load_icon(icon_name, img_size, TRUE);
            GtkWidget* btn = add_btn(table, name, icon, comment ? comment : name);
            if (icon)
                g_object_unref(icon);

            AppButton* app = g_new0(AppButton, 1);
            app->exec = exec;
            app->name = name;
            app->widget = btn;

            g_object_set_data(G_OBJECT(btn), "desktop-file-path", g_strdup(file_path));
            g_signal_connect(btn, "clicked", G_CALLBACK(on_app_btn_clicked), app);
            g_signal_connect(btn, "button-press-event", G_CALLBACK(on_app_btn_press_event), app);
            g_signal_connect(btn, "destroy", G_CALLBACK(free_app_btn), app);
        }
        else
        {
            g_free(name);
            g_free(exec);
        }
        g_free(icon_name);
        g_free(comment);
    }
    g_key_file_free(kf);
}

static void load_dir(const char* path, GtkWidget* table)
{
    GDir* dir = g_dir_open(path, 0, NULL);
    if (dir)
    {
        const char* file;
        while ((file = g_dir_read_name(dir)))
        {
            if (g_str_has_suffix(file, ".desktop"))
            {
                char* file_path = g_build_filename(path, file, NULL);
                load_desktop_entry(file_path, table);
                g_free(file_path);
            }
        }
        g_dir_close(dir);
    }
}

static void add_page(const char* title, const char* dir_name)
{
    GtkWidget *scrolled, *table, *label;
    char* path;

    scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

    table = exo_wrap_table_new(FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), table);

    label = gtk_label_new(title);
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrolled, label);

    path = g_build_filename(g_get_user_config_dir(), "lxlauncher", dir_name, NULL);
    load_dir(path, table);
    g_free(path);

    const gchar** dirs = (const gchar**)g_get_system_data_dirs();
    const gchar** d;
    for (d = dirs; *d; ++d)
    {
        path = g_build_filename(*d, "lxlauncher", dir_name, NULL);
        load_dir(path, table);
        g_free(path);
    }
    gtk_widget_show_all(scrolled);
}

static void load_pages()
{
    gchar** groups = g_key_file_get_groups(key_file, NULL);
    gchar** g;
    if (groups)
    {
        for (g = groups; *g; ++g)
        {
            if (strcmp(*g, "Main") != 0)
            {
                char* title = g_key_file_get_locale_string(key_file, *g, "Name", NULL, NULL);
                if (title)
                {
                    add_page(title, *g);
                    g_free(title);
                }
            }
        }
        g_strfreev(groups);
    }
}

int main(int argc, char* argv[])
{
#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    gtk_init(&argc, &argv);

    key_file = g_key_file_new();
    char* file = g_build_filename(g_get_user_config_dir(), "lxlauncher/settings.conf", NULL);
    if (!g_key_file_load_from_file(key_file, file, G_KEY_FILE_NONE, NULL))
    {
        g_free(file);
        file = g_build_filename(SYSCONFDIR, "xdg/lxlauncher/settings.conf", NULL);
        g_key_file_load_from_file(key_file, file, G_KEY_FILE_NONE, NULL);
    }
    g_free(file);

    file = g_build_filename(SYSCONFDIR, "xdg/lxlauncher/gtk.css", NULL);
    if (g_file_test(file, G_FILE_TEST_EXISTS) == TRUE)
    {
        GtkCssProvider *css = gtk_css_provider_new();
        gtk_css_provider_load_from_path(css, file, NULL);
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                GTK_STYLE_PROVIDER(css),
                GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
    g_free(file);
    
    file = g_build_filename(g_get_user_config_dir(), "lxlauncher/gtk.css", NULL);
    if (g_file_test(file, G_FILE_TEST_EXISTS) == TRUE)
    {
        GtkCssProvider *css = gtk_css_provider_new();
        gtk_css_provider_load_from_path(css, file, NULL);
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                GTK_STYLE_PROVIDER(css),
                GTK_STYLE_PROVIDER_PRIORITY_USER);
    }
    g_free(file);

    button_size = g_key_file_get_integer(key_file, \"Main\", \"BUTTON_SIZE\", NULL);
    img_size = g_key_file_get_integer(key_file, \"Main\", \"IMG_SIZE\", NULL);

    if (!button_size)
        button_size = BUTTON_SIZE_FALLBACK;
    if (!img_size)
        img_size = IMG_SIZE_FALLBACK;

    icon_size = gtk_icon_size_register("ALIcon", img_size, img_size);

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), _("Desktop Launcher"));
    
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GdkRectangle wa;
    get_working_area(gdk_screen_get_default(), &wa);
    gtk_window_move(GTK_WINDOW(main_window), wa.x, wa.y);
    gtk_window_set_default_size(GTK_WINDOW(main_window), wa.width, wa.height);

    notebook = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
    gtk_container_add(GTK_CONTAINER(main_window), notebook);

    load_pages();

    gtk_widget_show_all(main_window);
    gtk_main();

    g_key_file_free(key_file);
    return 0;
}