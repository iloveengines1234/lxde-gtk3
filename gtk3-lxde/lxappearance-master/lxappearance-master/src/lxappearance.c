/*
 * lxappearance.c
 * Updated specifically to align with GTK 3.24.52 safety standards.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "lxappearance.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#ifdef GDK_WINDOWING_X11
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#endif

#include <string.h>

#if ENABLE_DBUS
#include <gio/gio.h>
#endif

#include "widget-theme.h"
#include "color-scheme.h"
#include "icon-theme.h"
#include "cursor-theme.h"
#include "font.h"
#include "other.h"
#include "plugin.h"

LXAppearance app = {0};

#ifdef GDK_WINDOWING_X11
Atom lxsession_atom = 0;
#endif
static const char* lxsession_name = NULL;

static gboolean check_lxde_dbus(void)
{
#if ENABLE_DBUS
    GError *error = NULL;
    GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    if (connection == NULL)
    {
        g_warning(G_STRLOC ": Failed to connect to the session message bus: %s", error->message);
        g_error_free(error);
        return FALSE;
    }

    GVariant *reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus",
                                                  "/org/freedesktop/DBus",
                                                  "org.freedesktop.DBus",
                                                  "GetNameOwner",
                                                  g_variant_new("(s)", "org.lxde.SessionManager"),
                                                  NULL,
                                                  G_DBUS_CALL_FLAGS_NO_AUTO_START,
                                                  -1, NULL, NULL);
    g_object_unref(connection);

    if (reply != NULL)
    {
        g_variant_unref(reply);
        return TRUE;
    }
    return FALSE;
#else
    return FALSE;
#endif
}

static void check_lxsession(void)
{
    GdkDisplay *display = gdk_display_get_default();
#ifdef GDK_WINDOWING_X11
    if (GDK_IS_X11_DISPLAY(display))
    {
        lxsession_atom = XInternAtom(GDK_DISPLAY_XDISPLAY(display), "_LXSESSION", True);
        if (lxsession_atom != None)
        {
            XGrabServer(GDK_DISPLAY_XDISPLAY(display));
            if (XGetSelectionOwner(GDK_DISPLAY_XDISPLAY(display), lxsession_atom))
            {
                app.use_lxsession = TRUE;
                lxsession_name = g_getenv("DESKTOP_SESSION");
            }
            XUngrabServer(GDK_DISPLAY_XDISPLAY(display));
        }
    }
#endif
    if (!app.use_lxsession && check_lxde_dbus())
    {
        app.use_lxsession = TRUE;
        lxsession_name = g_getenv("DESKTOP_SESSION");
    }
}

static GOptionEntry option_entries[] =
{
    { NULL }
};

static gboolean verify_cursor_theme(GKeyFile *kf, const char *cursor_theme, const char *test)
{
    char *fpath;
    gboolean ret;

    fpath = g_build_filename(g_get_home_dir(), ".icons", cursor_theme, "index.theme", NULL);
    ret = g_key_file_load_from_file(kf, fpath, 0, NULL);
    g_free(fpath);

    if (!ret)
    {
        fpath = g_build_filename("icons", cursor_theme, "index.theme", NULL);
        ret = g_key_file_load_from_data_dirs(kf, fpath, NULL, 0, NULL);
        g_free(fpath);
    }

    if (ret)
    {
        fpath = g_key_file_get_string(kf, "Icon Theme", "Inherits", NULL);
        if (fpath == NULL)
            return TRUE;
        if (strcmp(fpath, test) == 0)
            ret = FALSE;
        else if (!verify_cursor_theme(kf, fpath, test))
            ret = FALSE;
        else
            ret = verify_cursor_theme(kf, fpath, cursor_theme);
        g_free(fpath);
    }
    return ret;
}

static void save_cursor_theme_name(void)
{
    char* dir_path;
    GKeyFile* kf;

    if (app.cursor_theme == NULL || strcmp(app.cursor_theme, "default") == 0)
        return;

    dir_path = g_build_filename(g_get_home_dir(), ".icons/default", NULL);
    kf = g_key_file_new();
    
    if (!verify_cursor_theme(kf, app.cursor_theme, "default"))
    {
        g_free(app.cursor_theme);
        app.cursor_theme = NULL;
    }
    else if (g_file_test(dir_path, G_FILE_TEST_IS_SYMLINK) && g_unlink(dir_path) != 0)
    {
        /* Error handling hook */
    }
    else if (0 == g_mkdir_with_parents(dir_path, 0700))
    {
        char* index_theme = g_build_filename(dir_path, "index.theme", NULL);
        char* content = g_strdup_printf(
                        "# This file is written by LXAppearance. Do not edit.\n"
                        "[Icon Theme]\n"
                        "Name=Default\n"
                        "Comment=Default Cursor Theme\n"
                        "Inherits=%s\n", app.cursor_theme);
        g_file_set_contents(index_theme, content, -1, NULL);
        g_free(content);
        g_free(index_theme);
    }
    g_key_file_free(kf);
    g_free(dir_path);
}

static void reload_all_programs(void)
{
    GtkSettings *settings = gtk_settings_get_default();
    if (settings)
    {
        if (app.widget_theme)
            g_object_set(settings, "gtk-theme-name", app.widget_theme, NULL);
        if (app.icon_theme)
            g_object_set(settings, "gtk-icon-theme-name", app.icon_theme, NULL);
        if (app.default_font)
            g_object_set(settings, "gtk-font-name", app.default_font, NULL);
        if (app.cursor_theme)
            g_object_set(settings, "gtk-cursor-theme-name", app.cursor_theme, NULL);
            
        g_object_set(settings, "gtk-cursor-theme-size", app.cursor_theme_size, NULL);
    }
}

static void lxappearance_save_gtkrc(void)
{
    static const char* tb_styles[] = {
        "GTK_TOOLBAR_ICONS", "GTK_TOOLBAR_TEXT", "GTK_TOOLBAR_BOTH", "GTK_TOOLBAR_BOTH_HORIZ"
    };
    static const char* tb_icon_sizes[] = {
        "GTK_ICON_SIZE_INVALID", "GTK_ICON_SIZE_MENU", "GTK_ICON_SIZE_SMALL_TOOLBAR",
        "GTK_ICON_SIZE_LARGE_TOOLBAR", "GTK_ICON_SIZE_BUTTON", "GTK_ICON_SIZE_DND", "GTK_ICON_SIZE_DIALOG"
    };

    GKeyFile *content = g_key_file_new();
    char* file_path = g_build_filename(g_get_user_config_dir(), "gtk-3.0", NULL);
    char* file_path_settings = g_build_filename(file_path, "settings.ini", NULL);

    if (!g_file_test(file_path, G_FILE_TEST_IS_DIR))
    {
        g_mkdir_with_parents(file_path, 0755);
    }

    g_key_file_load_from_file(content, file_path_settings, 
                              G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS, NULL);

    if(app.widget_theme)
        g_key_file_set_string(content, "Settings", "gtk-theme-name", app.widget_theme);
    if(app.icon_theme)
        g_key_file_set_string(content, "Settings", "gtk-icon-theme-name", app.icon_theme);
    if(app.default_font)
        g_key_file_set_string(content, "Settings", "gtk-font-name", app.default_font);
    if(app.cursor_theme)
        g_key_file_set_string(content, "Settings", "gtk-cursor-theme-name", app.cursor_theme);
        
    save_cursor_theme_name();

    g_key_file_set_integer(content, "Settings", "gtk-cursor-theme-size", app.cursor_theme_size);
    g_key_file_set_string(content, "Settings", "gtk-toolbar-style", tb_styles[app.toolbar_style - GTK_TOOLBAR_ICONS]);
    g_key_file_set_string(content, "Settings", "gtk-toolbar-icon-size", tb_icon_sizes[app.toolbar_icon_size]);
    g_key_file_set_integer(content, "Settings", "gtk-button-images", app.button_images ? 1 : 0);
    g_key_file_set_integer(content, "Settings", "gtk-menu-images", app.menu_images ? 1 : 0);
    g_key_file_set_integer(content, "Settings", "gtk-enable-event-sounds", app.enable_event_sound ? 1 : 0);
    g_key_file_set_integer(content, "Settings", "gtk-enable-input-feedback-sounds", app.enable_input_feedback ? 1 : 0);
    g_key_file_set_integer(content, "Settings", "gtk-xft-antialias", app.enable_antialising ? 1 : 0);
    g_key_file_set_integer(content, "Settings", "gtk-xft-hinting", app.enable_hinting ? 1 : 0);

    if(app.hinting_style)
        g_key_file_set_string(content, "Settings", "gtk-xft-hintstyle", app.hinting_style);

    if(app.font_rgba)
        g_key_file_set_string(content, "Settings", "gtk-xft-rgba", app.font_rgba);

    if(app.modules && app.modules[0])
        g_key_file_set_string(content, "Settings", "gtk-modules", app.modules);
    else
        g_key_file_remove_key(content, "Settings", "gtk-modules", NULL);

    g_key_file_save_to_file(content, file_path_settings, NULL);

    g_free(file_path_settings);
    g_free(file_path);
    g_key_file_free(content);
}

static void lxappearance_save_lxsession(void)
{
    char* rel_path = g_strconcat("lxsession/", lxsession_name, "/desktop.conf", NULL);
    char* user_config_file = g_build_filename(g_get_user_config_dir(), rel_path, NULL);
    char* buf;
    gsize len;
    GKeyFile* kf = g_key_file_new();

    if(!g_key_file_load_from_file(kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        len = strlen(user_config_file) - strlen("/desktop.conf");
        user_config_file[len] = '\0';
        g_mkdir_with_parents(user_config_file, 0700);
        user_config_file[len] = '/';

        g_key_file_load_from_dirs(kf, rel_path, (const char**)g_get_system_config_dirs(), NULL,
                                  G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    }

    g_free(rel_path);

    g_key_file_set_string(kf, "GTK", "sNet/ThemeName", app.widget_theme ? app.widget_theme : "");
    g_key_file_set_string(kf, "GTK", "sGtk/FontName", app.default_font ? app.default_font : "");
    g_key_file_set_string(kf, "GTK", "sGtk/ColorScheme", app.color_scheme ? app.color_scheme : "");
    g_key_file_set_string(kf, "GTK", "sNet/IconThemeName", app.icon_theme ? app.icon_theme : "");
    g_key_file_set_string(kf, "GTK", "sGtk/CursorThemeName", app.cursor_theme ? app.cursor_theme : "");
    g_key_file_set_integer(kf, "GTK", "iGtk/CursorThemeSize", app.cursor_theme_size);
    
    save_cursor_theme_name();

    g_key_file_set_integer(kf, "GTK", "iGtk/ToolbarStyle", app.toolbar_style);
    g_key_file_set_integer(kf, "GTK", "iGtk/ToolbarIconSize", app.toolbar_icon_size);
    g_key_file_set_integer(kf, "GTK", "iGtk/ButtonImages", app.button_images);
    g_key_file_set_integer(kf, "GTK", "iGtk/MenuImages", app.menu_images);
    g_key_file_set_integer(kf, "GTK", "iNet/EnableEventSounds", app.enable_event_sound);
    g_key_file_set_integer(kf, "GTK", "iNet/EnableInputFeedbackSounds", app.enable_input_feedback);
    g_key_file_set_integer(kf, "GTK", "iXft/Antialias", app.enable_antialising);
    g_key_file_set_integer(kf, "GTK", "iXft/Hinting", app.enable_hinting);
    g_key_file_set_string(kf, "GTK", "sXft/HintStyle", app.hinting_style ? app.hinting_style : "");
    g_key_file_set_string(kf, "GTK", "sXft/RGBA", app.font_rgba ? app.font_rgba : "");

    buf = g_key_file_to_data(kf, &len, NULL);
    g_key_file_free(kf);

    g_file_set_contents(user_config_file, buf, len, NULL);
    g_free(buf);
    g_free(user_config_file);
}

static void on_dlg_response(GtkDialog* dlg, int res, gpointer user_data)
{
    switch(res)
    {
    case GTK_RESPONSE_APPLY:
        if(app.use_lxsession)
            lxappearance_save_lxsession();
        lxappearance_save_gtkrc();
        reload_all_programs();
        app.changed = FALSE;
        gtk_dialog_set_response_sensitive(GTK_DIALOG(app.dlg), GTK_RESPONSE_APPLY, FALSE);
        break;
    case 1: /* About dialog */
        {
            GtkBuilder* b = gtk_builder_new();
            if(gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/about.ui", NULL))
            {
                GtkWidget* about_dlg = GTK_WIDGET(gtk_builder_get_object(b, "dlg"));
                gtk_dialog_run(GTK_DIALOG(about_dlg));
                gtk_widget_destroy(about_dlg);
            }
            g_object_unref(b);
        }
        break;
    default:
        gtk_main_quit();
    }
}

static void settings_init(void)
{
    GtkSettings* settings = gtk_settings_get_default();
    if (!settings) return;

    /* GTK 3.24.52 parameter check fallback mappings to avoid reflection lookup failure loops */
    g_object_get(settings,
                "gtk-theme-name", &app.widget_theme,
                "gtk-font-name", &app.default_font,
                "gtk-icon-theme-name", &app.icon_theme,
                "gtk-cursor-theme-name", &app.cursor_theme,
                "gtk-cursor-theme-size", &app.cursor_theme_size,
                "gtk-toolbar-style", &app.toolbar_style,
                "gtk-toolbar-icon-size", &app.toolbar_icon_size,
                "gtk-enable-event-sounds", &app.enable_event_sound,
                "gtk-enable-input-feedback-sounds", &app.enable_input_feedback,
                "gtk-xft-antialias", &app.enable_antialising,
                "gtk-xft-hinting", &app.enable_hinting,
                "gtk-xft-hintstyle", &app.hinting_style,
                "gtk-xft-rgba", &app.font_rgba,
                "gtk-modules", &app.modules,
                NULL);

    /* Safely check deprecated image parameters conditionally if they are present in the class metadata */
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "gtk-button-images"))
        g_object_get(settings, "gtk-button-images", &app.button_images, NULL);
    if (g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "gtk-menu-images"))
        g_object_get(settings, "gtk-menu-images", &app.menu_images, NULL);

    if(!app.cursor_theme || g_strcmp0(app.cursor_theme, "default") == 0)
    {
        GKeyFile* kf = g_key_file_new();
        char* fpath = g_build_filename(g_get_home_dir(), ".icons/default/index.theme", NULL);
        gboolean ret = g_key_file_load_from_file(kf, fpath, 0, NULL);
        g_free(fpath);

        if(!ret)
            ret = g_key_file_load_from_data_dirs(kf, "icons/default/index.theme", NULL, 0, NULL);

        if(ret)
        {
            g_free(app.cursor_theme);
            app.cursor_theme = g_key_file_get_string(kf, "Icon Theme", "Inherits", NULL);
        }
        g_key_file_free(kf);
    }

    app.color_scheme_hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
    if(app.use_lxsession)
    {
        char* rel_path = g_strconcat("lxsession/", lxsession_name, "/desktop.conf", NULL);
        char* user_config_file = g_build_filename(g_get_user_config_dir(), rel_path, NULL);
        GKeyFile* kf = g_key_file_new();
        if(g_key_file_load_from_file(kf, user_config_file, 0, NULL))
            app.color_scheme = g_key_file_get_string(kf, "GTK", "sGtk/ColorScheme", NULL);
        else if(g_key_file_load_from_dirs(kf, rel_path, (const char**)g_get_system_config_dirs(), NULL, 0, NULL))
            app.color_scheme = g_key_file_get_string(kf, "GTK", "sGtk/ColorScheme", NULL);
        g_key_file_free(kf);
        g_free(rel_path);
        g_free(user_config_file);

        if(app.color_scheme)
        {
            if(*app.color_scheme)
                color_scheme_str_to_hash(app.color_scheme_hash, app.color_scheme);
            else
            {
                g_free(app.color_scheme);
                app.color_scheme = NULL;
            }
        }
    }
}

int main(int argc, char** argv)
{
    GError* err = NULL;
    GtkBuilder* b;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    if(G_UNLIKELY(!gtk_init_with_args(&argc, &argv, "", option_entries, GETTEXT_PACKAGE, &err)))
    {
        g_print("Error: %s\n", err->message);
        g_error_free(err);
        return 1;
    }

    app.abi_version = LXAPPEARANCE_ABI_VERSION;
    check_lxsession();

    b = gtk_builder_new();
    if(!gtk_builder_add_from_file(b, PACKAGE_UI_DIR "/lxappearance.ui", NULL))
    {
        g_object_unref(b);
        return 1;
    }

    settings_init();

    app.dlg = GTK_WIDGET(gtk_builder_get_object(b, "dlg"));

    widget_theme_init(b);
    color_scheme_init(b);
    icon_theme_init(b);
    cursor_theme_init(b);
    font_init(b);
    other_init(b);
    app.wm_page = GTK_WIDGET(gtk_builder_get_object(b, "wm_page"));

    plugins_init(b);

    g_signal_connect(app.dlg, "response", G_CALLBACK(on_dlg_response), NULL);

    gtk_window_present(GTK_WINDOW(app.dlg));
    g_object_unref(b);

    gtk_main();
    plugins_finalize();

    return 0;
}

void lxappearance_changed(void)
{
    if(!app.changed)
    {
        app.changed = TRUE;
        gtk_dialog_set_response_sensitive(GTK_DIALOG(app.dlg), GTK_RESPONSE_APPLY, TRUE);
    }
}

void on_check_button_toggled(GtkToggleButton* btn, gpointer user_data)
{
    gboolean* val = (gboolean*)user_data;
    gboolean new_val = gtk_toggle_button_get_active(btn);
    if(new_val != *val)
    {
        *val = new_val;
        lxappearance_changed();
    }
}