/*
 *      lxinput.c
 *
 *      Copyright 2009-2011 PCMan <pcman.tw@gmail.com>
 *      Copyright 2009 martyj19 <martyj19@comcast.net>
 *      Copyright 2011-2013 Julien Lavergne <julien.lavergne@gmail.com>
 *      Copyright 2014 Andriy Grytsenko <andrej@rep.kiev.ua>
 *      Copyright 2023 Ingo Brückl
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

#include <config.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <math.h>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

static GtkWidget *dlg;
static GtkRange *mouse_accel;
static GtkRange *mouse_threshold;
static GtkToggleButton* mouse_left_handed;
static GtkRange *kb_delay;
static GtkRange *kb_interval;
static GtkToggleButton* kb_beep;
static GtkButton* kb_layout;
static GtkFrame* kb_layout_frame;

static int accel = 20, old_accel = 20;
static int threshold = 10, old_threshold = 10;
static gboolean left_handed = FALSE, old_left_handed = FALSE;

static int delay = 500, old_delay = 500;
static int interval = 30, old_interval = 30;
static gboolean beep = TRUE, old_beep = TRUE;

static char* user_config_file = NULL;
static char* rel_path = NULL;
static GKeyFile* kf = NULL;

/* Modern display/screen helper logic to protect against multi-head server faults */
static Display* get_current_display(void)
{
    GdkDisplay *display = gdk_display_get_default();
    if (!display) return NULL;
    return GDK_DISPLAY_XDISPLAY(display);
}

static void on_mouse_accel_changed(GtkRange* range, gpointer user_data)
{
    Display *dpy = get_current_display();
    if (!dpy) return;

    accel = (int)(gtk_range_get_value(range) * 10);
    XChangePointerControl(dpy, True, False, accel, 10, 0);
}

static void on_mouse_threshold_changed(GtkRange* range, gpointer user_data)
{
    Display *dpy = get_current_display();
    if (!dpy) return;

    threshold = (int)gtk_range_get_value(range);
    XChangePointerControl(dpy, False, True, 0, 10, threshold);
}

static void on_kb_range_changed(GtkRange* range, gpointer user_data)
{
    Display *dpy = get_current_display();
    if (!dpy) return;

    int *val = (int*)user_data;
    *val = (int)gtk_range_get_value(range);
    XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, delay, interval);
}

#define DEFAULT_PTR_MAP_SIZE 128
static void set_left_handed_mouse(void)
{
    Display *dpy = get_current_display();
    if (!dpy) return;

    unsigned char *buttons;
    gint n_buttons, i;
    gint idx_1 = -1, idx_3 = -1;

    buttons = g_alloca(DEFAULT_PTR_MAP_SIZE);
    n_buttons = XGetPointerMapping(dpy, buttons, DEFAULT_PTR_MAP_SIZE);
    if (n_buttons > DEFAULT_PTR_MAP_SIZE)
    {
        buttons = g_alloca(n_buttons);
        n_buttons = XGetPointerMapping(dpy, buttons, n_buttons);
    }

    for (i = 0; i < n_buttons; i++)
    {
        if (buttons[i] == 1)
            idx_1 = i;
        else if (buttons[i] == ((n_buttons < 3) ? 2 : 3))
            idx_3 = i;
    }

    if (idx_1 != -1 && idx_3 != -1)
    {
        if ((left_handed && idx_1 < idx_3) || (!left_handed && idx_1 > idx_3))
        {
            buttons[idx_1] = ((n_buttons < 3) ? 2 : 3);
            buttons[idx_3] = 1;
            XSetPointerMapping(dpy, buttons, n_buttons);
        }
    }
}

static void on_left_handed_toggle(GtkToggleButton* btn, gpointer user_data)
{
    left_handed = gtk_toggle_button_get_active(btn);
    set_left_handed_mouse();
}

static void on_kb_beep_toggle(GtkToggleButton* btn, gpointer user_data)
{
    Display *dpy = get_current_display();
    if (!dpy) return;

    XKeyboardControl values;
    beep = gtk_toggle_button_get_active(btn);
    values.bell_percent = beep ? -1 : 0;
    XChangeKeyboardControl(dpy, KBBellPercent, &values);
}

static const gchar* detect_keymap_program(void)
{
    if (g_find_program_in_path("lxkeymap"))
        return "lxkeymap";
    return NULL;
}

static void on_kb_layout_clicked(GtkButton *button, gpointer user_data)
{
    GError *error = NULL;
    const gchar *program = detect_keymap_program();

    if (program)
    {
        /* Fixed fatal deprecated synchronous thread-stalling hazard */
        g_spawn_command_line_async(program, &error);
        if (error)
        {
            g_warning("Failed to run layout selector tool: %s", error->message);
            g_error_free(error);
        }
    }
}

static void set_range_stops(GtkRange* range, int chunk_interval)
{
    gtk_range_set_increments(range, chunk_interval, chunk_interval * 5);
}

static void load_settings(void)
{
    const char* session_name = g_getenv("DESKTOP_SESSION");
    if (!session_name)
        session_name = "LXDE";

    rel_path = g_strconcat("lxsession/", session_name, "/desktop.conf", NULL);
    user_config_file = g_build_filename(g_get_user_config_dir(), rel_path, NULL);
    kf = g_key_file_new();

    if (!g_key_file_load_from_file(kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
    {
        g_key_file_load_from_dirs(kf, rel_path, (const char**)g_get_system_config_dirs(), NULL,
                                  G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
    }

    int val;
    val = g_key_file_get_integer(kf, "Mouse", "AccFactor", NULL);
    if (val > 0) old_accel = accel = val;

    val = g_key_file_get_integer(kf, "Mouse", "AccThreshold", NULL);
    if (val > 0) old_threshold = threshold = val;

    old_left_handed = left_handed = g_key_file_get_boolean(kf, "Mouse", "LeftHanded", NULL);

    val = g_key_file_get_integer(kf, "Keyboard", "Delay", NULL);
    if (val > 0) old_delay = delay = val;
    val = g_key_file_get_integer(kf, "Keyboard", "Interval", NULL);
    if (val > 0) old_interval = interval = val;

    if (g_key_file_has_key(kf, "Keyboard", "Beep", NULL))
        old_beep = beep = g_key_file_get_boolean(kf, "Keyboard", "Beep", NULL);
}

static void on_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if (response_id == GTK_RESPONSE_OK)
    {
        gsize len;
        char *str = NULL;

        if (!g_key_file_load_from_file(kf, user_config_file, G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
        {
            char *parent_dir = g_path_get_dirname(user_config_file);
            g_mkdir_with_parents(parent_dir, 0700);
            g_free(parent_dir);

            g_key_file_load_from_dirs(kf, rel_path, (const char**)g_get_system_config_dirs(), NULL,
                                      G_KEY_FILE_KEEP_COMMENTS|G_KEY_FILE_KEEP_TRANSLATIONS, NULL);
        }

        g_key_file_set_integer(kf, "Mouse", "AccFactor", accel);
        g_key_file_set_integer(kf, "Mouse", "AccThreshold", threshold);
        g_key_file_set_integer(kf, "Mouse", "LeftHanded", !!left_handed);

        g_key_file_set_integer(kf, "Keyboard", "Delay", delay);
        g_key_file_set_integer(kf, "Keyboard", "Interval", interval);
        g_key_file_set_integer(kf, "Keyboard", "Beep", !!beep);

        str = g_key_file_to_data(kf, &len, NULL);
        g_file_set_contents(user_config_file, str, len, NULL);
        g_free(str);

        char *autostart_dir = g_build_filename(g_get_user_config_dir(), "autostart", NULL);
        char *autostart_file = g_build_filename(autostart_dir, "LXinput-setup.desktop", NULL);
        
        if (g_mkdir_with_parents(autostart_dir, 0755) == 0)
        {
            str = g_strdup_printf("[Desktop Entry]\n"
                                  "Type=Application\n"
                                  "Name=LXInput autostart\n"
                                  "Comment=Setup keyboard and mouse using settings done in LXInput\n"
                                  "NoDisplay=true\n"
                                  "Exec=sh -c 'xset m %d/10 %d r rate %d %d b %s%s'\n"
                                  "NotShowIn=GNOME;KDE;XFCE;\n",
                                  accel, threshold, delay, interval,
                                  beep ? "on" : "off",
                                  left_handed ? ";xmodmap -e \"pointer = 3 2 1\"" : "");
            g_file_set_contents(autostart_file, str, -1, NULL);
            g_free(str);
        }
        g_free(autostart_dir);
        g_free(autostart_file);
    }
    else
    {
        Display *dpy = get_current_display();
        if (dpy)
        {
            XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, old_delay, old_interval);
            XChangePointerControl(dpy, True, True, old_accel, 10, old_threshold);
            left_handed = old_left_handed;
            set_left_handed_mouse();
        }
    }

    gtk_widget_destroy(GTK_WIDGET(dialog));
    g_free(user_config_file);
    g_free(rel_path);
    g_key_file_free(kf);
    gtk_main_quit();
}

int main(int argc, char** argv)
{
    GtkBuilder* builder;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    gtk_init(&argc, &argv);
    gtk_icon_theme_prepend_search_path(gtk_icon_theme_get_default(), PACKAGE_DATA_DIR);

    load_settings();

    builder = gtk_builder_new();
    gtk_builder_add_from_file(builder, PACKAGE_DATA_DIR "/lxinput.ui", NULL);
    dlg = (GtkWidget*)gtk_builder_get_object(builder, "dlg");
    gtk_dialog_set_alternative_button_order((GtkDialog*)dlg, GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

    mouse_accel = (GtkRange*)gtk_builder_get_object(builder,"mouse_accel");
    mouse_threshold = (GtkRange*)gtk_builder_get_object(builder,"mouse_threshold");
    mouse_left_handed = (GtkToggleButton*)gtk_builder_get_object(builder,"left_handed");

    kb_delay = (GtkRange*)gtk_builder_get_object(builder,"kb_delay");
    kb_interval = (GtkRange*)gtk_builder_get_object(builder,"kb_interval");
    kb_beep = (GtkToggleButton*)gtk_builder_get_object(builder,"beep");
    kb_layout = (GtkButton*)gtk_builder_get_object(builder,"keyboard_layout");

    const gchar *program = detect_keymap_program();
    if (program == NULL)
    {
        kb_layout_frame = (GtkFrame*)gtk_builder_get_object(builder,"keyboard_layout_frame");
        if (kb_layout_frame)
            gtk_widget_set_visible(GTK_WIDGET(kb_layout_frame), FALSE);
    }
    else
    {
        gtk_button_set_label(kb_layout, program);
    }

    g_object_unref(builder);

    gtk_range_set_value(mouse_accel, (gdouble)accel / 10.0);
    gtk_range_set_value(mouse_threshold, threshold);
    gtk_toggle_button_set_active(mouse_left_handed, left_handed);

    gtk_range_set_value(kb_delay, delay);
    gtk_range_set_value(kb_interval, interval);
    gtk_toggle_button_set_active(kb_beep, beep);

    set_range_stops(mouse_accel, 10);
    g_signal_connect(mouse_accel, "value-changed", G_CALLBACK(on_mouse_accel_changed), NULL);
    set_range_stops(mouse_threshold, 10);
    g_signal_connect(mouse_threshold, "value-changed", G_CALLBACK(on_mouse_threshold_changed), NULL);
    g_signal_connect(mouse_left_handed, "toggled", G_CALLBACK(on_left_handed_toggle), NULL);

    set_range_stops(kb_delay, 10);
    g_signal_connect(kb_delay, "value-changed", G_CALLBACK(on_kb_range_changed), &delay);
    set_range_stops(kb_interval, 10);
    g_signal_connect(kb_interval, "value-changed", G_CALLBACK(on_kb_range_changed), &interval);
    g_signal_connect(kb_beep, "toggled", G_CALLBACK(on_kb_beep_toggle), NULL);
    g_signal_connect(kb_layout, "clicked", G_CALLBACK(on_kb_layout_clicked), NULL);

    g_signal_connect(dlg, "response", G_CALLBACK(on_dialog_response), NULL);
    gtk_widget_show_all(dlg);

    gtk_main();
    return 0;
}