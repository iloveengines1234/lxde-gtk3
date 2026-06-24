/**
 * Copyright (c) 2010 LxDE Developers, see the file AUTHORS for details.
 * Copyright (C) 2020 Ingo Brückl
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


#include <config.h>
#include <locale.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib/gi18n.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include "lxsession-logout-dbus-interface.h"

/* Command parameters. */
static char * prompt = NULL;
static char * banner_side = NULL;
static char * banner_path = NULL;
static char **hide_button = NULL;

static GOptionEntry opt_entries[] =
{
    { "prompt", 'p', 0, G_OPTION_ARG_STRING, &prompt, N_("Custom message to show on the dialog"), N_("message") },
    { "banner", 'b', 0, G_OPTION_ARG_STRING, &banner_path, N_("Banner to show on the dialog"), N_("image file") },
    { "side", 's', 0, G_OPTION_ARG_STRING, &banner_side, N_("Position of the banner"), "top|left|right|bottom" },
    { "hide-button", 'H', 0, G_OPTION_ARG_STRING_ARRAY, &hide_button, N_("Hide button"), "shutdown|reboot|logout|hibernate|suspend|switch_user|lock_screen" },
    { NULL }
};

typedef struct {
    GPid lxsession_pid;			/* Process ID of lxsession */
    GtkWidget * error_label;		/* Text of an error, if we get one */

    int shutdown_available : 1;		/* Shutdown is available */
    int reboot_available : 1;		/* Reboot is available */
    int suspend_available : 1;		/* Suspend is available */
    int hibernate_available : 1;	/* Hibernate is available */
    int switch_user_available : 1;	/* Switch User is available */

    int shutdown_systemd : 1;		/* Shutdown is available via systemd */
    int reboot_systemd : 1;		/* Reboot is available via systemd */
    int suspend_systemd : 1;		/* Suspend is available via systemd */
    int hibernate_systemd : 1;		/* Hibernate is available via systemd */
    int shutdown_ConsoleKit : 1;	/* Shutdown is available via ConsoleKit */
    int reboot_ConsoleKit : 1;		/* Reboot is available via ConsoleKit */
    int suspend_ConsoleKit : 1;		/* Suspend is available via ConsoleKit */
    int hibernate_ConsoleKit : 1;	/* Hibernate is available via ConsoleKit */
    int suspend_UPower : 1;		/* Suspend is available via UPower */
    int hibernate_UPower : 1;		/* Hibernate is available via UPower */
    int switch_user_GDM : 1;		/* Switch User is available via GDM */
    int switch_user_LIGHTDM : 1;	/* Switch User is available via GDM */
    int switch_user_KDM : 1;		/* Switch User is available via LIGHTDM */
    int switch_user_LXDM : 1;		/* Switch User is available via LXDM */
    int ltsp : 1;			/* Shutdown and reboot is accomplished via LTSP */

    int lock_screen : 1;                /* Lock screen available */

} HandlerContext;

static gboolean lock_screen(void);
static const gchar* determine_lock_screen(void);
static gboolean verify_running(const char * display_manager, const char * executable);
static void logout_clicked(GtkButton * button, HandlerContext * handler_context);
static void change_root_property(GtkWidget* w, const char* prop_name, const char* value);
static void shutdown_clicked(GtkButton * button, HandlerContext * handler_context);
static void reboot_clicked(GtkButton * button, HandlerContext * handler_context);
static void suspend_clicked(GtkButton * button, HandlerContext * handler_context);
static void hibernate_clicked(GtkButton * button, HandlerContext * handler_context);
static void switch_user_clicked(GtkButton * button, HandlerContext * handler_context);
static void cancel_clicked(GtkButton * button, gpointer user_data);
static GtkPositionType get_banner_position(void);
static GdkPixbuf * get_background_pixbuf(GtkWidget *widget);
static gboolean draw(GtkWidget * widget, cairo_t * cr, GdkPixbuf * pixbuf);

static gboolean lock_screen(void)
{
    const gchar* program = determine_lock_screen();

    if (program)
    {
        g_spawn_command_line_async(program, NULL);
        return TRUE;
    }
    return FALSE;
}

static const gchar* determine_lock_screen(void)
{
    const gchar* program = NULL;

    if (g_find_program_in_path("lxlock"))
    {
        program = "lxlock";
    }
    else if (g_find_program_in_path("xdg-screensaver"))
    {
        program = "xdg-screensaver lock";
    }
    return program;
}

static gboolean verify_running(const char * display_manager, const char * executable)
{
    gchar * full_path = g_find_program_in_path(executable);
    if (full_path != NULL)
    {
        g_free(full_path);

        char buffer[PATH_MAX];
        sprintf(buffer, "/var/run/%s.pid", display_manager);

        if (!g_file_test (buffer, G_FILE_TEST_IS_REGULAR))
            sprintf(buffer, "/var/run/%s/%s.pid", display_manager, display_manager);

        int fd = open(buffer, O_RDONLY);
        if (fd >= 0)
        {
            ssize_t length = read(fd, buffer, sizeof(buffer));
            close(fd);
            if (length > 0)
            {
                buffer[length] = '\0';
                pid_t pid = atoi(buffer);
                if (pid > 0)
                {
                    sprintf(buffer, "/proc/%d/cmdline", pid);

                    int fd = open(buffer, O_RDONLY);
                    if (fd >= 0)
                    {
                        ssize_t length = read(fd, buffer, sizeof(buffer));
                        close(fd);
                        if (length > 0)
                        {
                            buffer[length] = '\0';
                            if (strstr(buffer, display_manager) != NULL)
                                return TRUE;
                        }
                    }
                }
            }
        }
    }
    return FALSE;
}

static void logout_clicked(GtkButton * button, HandlerContext * handler_context)
{
    if (handler_context->lxsession_pid != 0)
    {
        kill(handler_context->lxsession_pid, SIGTERM);
    }
    else
    {
        g_spawn_command_line_async("openbox --exit", NULL);
    }
    gtk_main_quit();
}

static void change_root_property(GtkWidget* w, const char* prop_name, const char* value)
{
    GdkDisplay* dpy = gtk_widget_get_display(w);
    GdkWindow* root = gdk_screen_get_root_window(gdk_display_get_default_screen(dpy));
    XChangeProperty(GDK_DISPLAY_XDISPLAY(dpy), GDK_WINDOW_XID(root),
                      XInternAtom(GDK_DISPLAY_XDISPLAY(dpy), prop_name, False), XA_STRING, 8,
                      PropModeReplace, (unsigned char*) value, strlen(value) + 1);
}

static void shutdown_clicked(GtkButton * button, HandlerContext * handler_context)
{
    GError *err = NULL;
    gtk_label_set_text(GTK_LABEL(handler_context->error_label), NULL);

    if (handler_context->ltsp)
    {
        change_root_property(GTK_WIDGET(button), "LTSP_LOGOUT_ACTION", "HALT");
        if (handler_context->lxsession_pid != 0)
        {
            kill(handler_context->lxsession_pid, SIGTERM);
        }
    }
    else if (handler_context->shutdown_ConsoleKit)
        dbus_ConsoleKit_PowerOff(&err);
    else if (handler_context->shutdown_systemd)
        dbus_systemd_PowerOff(&err);

	if (err)
	{
		gtk_label_set_text(GTK_LABEL(handler_context->error_label), err->message);
		g_error_free (err);
	}
	else
    {
        gtk_main_quit();
    }
}

static void reboot_clicked(GtkButton * button, HandlerContext * handler_context)
{
    GError *err = NULL;
    gtk_label_set_text(GTK_LABEL(handler_context->error_label), NULL);

    if (handler_context->ltsp)
    {
        change_root_property(GTK_WIDGET(button), "LTSP_LOGOUT_ACTION", "REBOOT");
        if (handler_context->lxsession_pid != 0)
        {
            kill(handler_context->lxsession_pid, SIGTERM);
        }
    }
    else if (handler_context->reboot_ConsoleKit)
        dbus_ConsoleKit_Reboot(&err);
    else if (handler_context->reboot_systemd)
        dbus_systemd_Reboot(&err);

	if (err)
	{
		gtk_label_set_text(GTK_LABEL(handler_context->error_label), err->message);
		g_error_free (err);
	}
	else
    {
        gtk_main_quit();
    }
}

static void suspend_clicked(GtkButton * button, HandlerContext * handler_context)
{
    GError *err = NULL;
    gtk_label_set_text(GTK_LABEL(handler_context->error_label), NULL);

    lock_screen();
    if (handler_context->suspend_UPower)
        dbus_UPower_Suspend(&err);
    else if (handler_context->suspend_ConsoleKit)
        dbus_ConsoleKit_Suspend(&err);
    else if (handler_context->suspend_systemd)
        dbus_systemd_Suspend(&err);

	if (err)
	{
		gtk_label_set_text(GTK_LABEL(handler_context->error_label), err->message);
		g_error_free (err);
	}
	else
    {
        gtk_main_quit();
    }
}

static void hibernate_clicked(GtkButton * button, HandlerContext * handler_context)
{
    GError *err = NULL;
    gtk_label_set_text(GTK_LABEL(handler_context->error_label), NULL);

    lock_screen();
    if (handler_context->hibernate_UPower)
        dbus_UPower_Hibernate(&err);
    else if (handler_context->hibernate_ConsoleKit)
        dbus_ConsoleKit_Hibernate(&err);
    else if (handler_context->hibernate_systemd)
        dbus_systemd_Hibernate(&err);

	if (err)
	{
		gtk_label_set_text(GTK_LABEL(handler_context->error_label), err->message);
		g_error_free (err);
	}
	else
    {
        gtk_main_quit();
    }
}

static void switch_user_clicked(GtkButton * button, HandlerContext * handler_context)
{
    GError *err = NULL;
    gtk_label_set_text(GTK_LABEL(handler_context->error_label), NULL);

    lock_screen();
    if (handler_context->switch_user_GDM)
        g_spawn_command_line_sync("gdmflexiserver --startnew", NULL, NULL, NULL, NULL);
    else if (handler_context->switch_user_KDM)
        g_spawn_command_line_sync("kdmctl reserve", NULL, NULL, NULL, NULL);
    else if (handler_context->switch_user_LIGHTDM)
        dbus_Lightdm_SwitchToGreeter(&err);
    else if(handler_context->switch_user_LXDM)
        g_spawn_command_line_sync("lxdm-binary -c USER_SWITCH", NULL, NULL, NULL, NULL);

	if (err)
	{
		gtk_label_set_text(GTK_LABEL(handler_context->error_label), err->message);
		g_error_free (err);
	}
	else
    {
        gtk_main_quit();
    }
}

static void lock_screen_clicked(GtkButton * button, HandlerContext * handler_context)
{
    gtk_label_set_text(GTK_LABEL(handler_context->error_label), NULL);

    lock_screen();
    gtk_main_quit();
}

static void cancel_clicked(GtkButton * button, gpointer user_data)
{
    gtk_main_quit();
}

static GtkPositionType get_banner_position(void)
{
    if (banner_side != NULL)
    {
        if (strcmp(banner_side, "right") == 0)
            return GTK_POS_RIGHT;
        if (strcmp(banner_side, "top") == 0)
            return GTK_POS_TOP;
        if (strcmp(banner_side, "bottom") == 0)
            return GTK_POS_BOTTOM;
    }
    return GTK_POS_LEFT;
}

/* Updated for GTK 3.24: Multi-monitor safe dimension checking logic */
static GdkPixbuf * get_background_pixbuf(GtkWidget *widget)
{
    GdkDisplay *display = gtk_widget_get_display(widget);
    GdkScreen *screen = gdk_display_get_default_screen(display);
    GdkWindow *root = gdk_screen_get_root_window(screen);
    GdkMonitor *monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(widget));
    GdkRectangle geom;

    if (monitor)
        gdk_monitor_get_geometry(monitor, &geom);
    else {
        geom.x = 0; geom.y = 0;
        geom.width = 800; geom.height = 600; /* Fallback baseline */
    }

    GdkPixbuf *pixbuf = gdk_pixbuf_get_from_window(root, geom.x, geom.y, geom.width, geom.height);

    if (pixbuf != NULL)
    {
        unsigned char * pixels = gdk_pixbuf_get_pixels(pixbuf);
        int width = gdk_pixbuf_get_width(pixbuf);
        int height = gdk_pixbuf_get_height(pixbuf);
        int pixel_stride = ((gdk_pixbuf_get_has_alpha(pixbuf)) ? 4 : 3);
        int row_stride = gdk_pixbuf_get_rowstride(pixbuf);
        int y;
        for (y = 0; y < height; y += 1)
        {
            unsigned char * p = pixels;
            int x;
            for (x = 0; x < width; x += 1)
            {
                p[0] = p[0] / 2;
                p[1] = p[1] / 2;
                p[2] = p[2] / 2;
                p += pixel_stride;
            }
            pixels += row_stride;
        }
    }
    return pixbuf;
}

static gboolean draw(GtkWidget * widget, cairo_t * cr, GdkPixbuf * pixbuf)
{
    gint x = 0, y = 0;

    if (pixbuf != NULL)
    {
       GdkWindow *window = gtk_widget_get_window(widget);
       if (window)
       {
           gdk_window_get_origin(window, &x, &y);
       }
       gdk_cairo_set_source_pixbuf(cr, pixbuf, -x, -y);
       cairo_paint(cr);
    }
    return FALSE;
}

static char lockfile[PATH_MAX];

static void main_at_exit(void)
{
    unlink(lockfile);
}

int main(int argc, char * argv[])
{
#ifdef ENABLE_NLS
    setlocale(LC_ALL, "");
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);
#endif

    HandlerContext handler_context;
    memset(&handler_context, 0, sizeof(handler_context));

    const char * p = g_getenv("_LXSESSION_PID");
    if (p != NULL) handler_context.lxsession_pid = atoi(p);

    sprintf(lockfile, "/tmp/.lxsession-logout-%d.lock", handler_context.lxsession_pid);
    int fd = open(lockfile, O_RDONLY|O_CREAT, 00600);
    if (fd >= 0)
    {
        if (flock(fd, LOCK_EX | LOCK_NB))
        {
            exit(EXIT_FAILURE);
        }
    }
    atexit(main_at_exit);

    if (dbus_systemd_CanPowerOff())
    {
        handler_context.shutdown_available = TRUE;
        handler_context.shutdown_systemd = TRUE;
    }
    if (dbus_systemd_CanReboot())
    {
        handler_context.reboot_available = TRUE;
        handler_context.reboot_systemd = TRUE;
    }
    if (dbus_systemd_CanSuspend())
    {
        handler_context.suspend_available = TRUE;
        handler_context.suspend_systemd = TRUE;
    }
    if (dbus_systemd_CanHibernate())
    {
        handler_context.hibernate_available = TRUE;
        handler_context.hibernate_systemd = TRUE;
    }

    if (!handler_context.shutdown_available && dbus_ConsoleKit_CanPowerOff())
    {
        handler_context.shutdown_available = TRUE;
        handler_context.shutdown_ConsoleKit = TRUE;
    }
    if (!handler_context.reboot_available && dbus_ConsoleKit_CanReboot())
    {
        handler_context.reboot_available = TRUE;
        handler_context.reboot_ConsoleKit = TRUE;
    }
    if (!handler_context.suspend_available && dbus_ConsoleKit_CanSuspend())
    {
        handler_context.suspend_available = TRUE;
        handler_context.suspend_ConsoleKit = TRUE;
    }
    if (!handler_context.hibernate_available && dbus_ConsoleKit_CanHibernate())
    {
        handler_context.hibernate_available = TRUE;
        handler_context.hibernate_ConsoleKit = TRUE;
    }

    if (!handler_context.suspend_available && dbus_UPower_CanSuspend())
    {
        handler_context.suspend_available = TRUE;
        handler_context.suspend_UPower = TRUE;
    }
    if (!handler_context.hibernate_available && dbus_UPower_CanHibernate())
    {
        handler_context.hibernate_available = TRUE;
        handler_context.hibernate_UPower = TRUE;
    }

    if (verify_running("gdm", "gdmflexiserver"))
    {
        handler_context.switch_user_available = TRUE;
        handler_context.switch_user_GDM = TRUE;
    }

    if (verify_running("gdm3", "gdmflexiserver"))
    {
        handler_context.switch_user_available = TRUE;
        handler_context.switch_user_GDM = TRUE;
    }

    if (g_getenv("XDG_SEAT_PATH"))
    {
        handler_context.switch_user_available = TRUE;
        handler_context.switch_user_LIGHTDM = TRUE;
    }

    if (verify_running("kdm", "kdmctl"))
    {
        handler_context.switch_user_available = TRUE;
        handler_context.switch_user_KDM = TRUE;
    }

    if (verify_running("lxdm", "lxdm-binary"))
    {
        handler_context.switch_user_available = TRUE;
        handler_context.switch_user_LXDM = TRUE;
    }

    if (g_getenv("LTSP_CLIENT"))
    {
        handler_context.ltsp = TRUE;
        handler_context.shutdown_available = TRUE;
        handler_context.reboot_available = TRUE;
    }

    const gchar* very_lock_screen = determine_lock_screen();
    if (very_lock_screen)
    {
        handler_context.lock_screen = TRUE;
    }

    GOptionContext * context = g_option_context_new("");
    g_option_context_add_main_entries(context, opt_entries, GETTEXT_PACKAGE);
    g_option_context_add_group(context, gtk_get_option_group(TRUE));
    GError * err = NULL;
    if ( ! g_option_context_parse(context, &argc, &argv, &err))
    {
        g_print(_("Error: %s\n"), err->message);
        g_error_free(err);
        return 1;
    }
    g_option_context_free(context);

    gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(), PACKAGE_DATA_DIR "/lxsession/images");

    /* Initialize root layout frame containers */
    GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(window));
    gtk_widget_set_app_paintable(window, TRUE);
    
    GdkPixbuf * pixbuf = get_background_pixbuf(window);
    g_signal_connect(G_OBJECT(window), "draw", G_CALLBACK(draw), pixbuf);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget* center_area = gtk_event_box_new();
    gtk_widget_set_halign(center_area, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center_area, GTK_ALIGN_CENTER);
    gtk_container_add(GTK_CONTAINER(window), center_area);

    GtkWidget* center_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(center_vbox), 12);
    gtk_container_add(GTK_CONTAINER(center_area), center_vbox);

    GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);

    if (hide_button != NULL) {
      for (guint i = 0; i < g_strv_length(hide_button); i++) {
        if (strcmp(hide_button[i], "shutdown") == 0) {
          handler_context.shutdown_available = FALSE;
        }
        if (strcmp(hide_button[i], "reboot") == 0) {
          handler_context.reboot_available = FALSE;
        }
        if (strcmp(hide_button[i], "hibernate") == 0) {
          handler_context.hibernate_available = FALSE;
        }
        if (strcmp(hide_button[i],"suspend") == 0) {
          handler_context.suspend_available = FALSE;
        }
        if (strcmp(hide_button[i], "switch_user") == 0) {
          handler_context.switch_user_available = FALSE;
        }
        if (strcmp(hide_button[i], "lock_screen") == 0) {
          handler_context.lock_screen = FALSE;
        }
      }
    }

    if (banner_path != NULL)
    {
        GtkWidget * banner_image = gtk_image_new_from_file(banner_path);
        GtkPositionType banner_position = get_banner_position();

        switch (banner_position)
        {
            case GTK_POS_LEFT:
            case GTK_POS_RIGHT:
                {
                GtkWidget * box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
                gtk_box_pack_start(GTK_BOX(center_vbox), box, FALSE, FALSE, 0);

                gtk_widget_set_halign(banner_image, GTK_ALIGN_CENTER);
                gtk_widget_set_valign(banner_image, GTK_ALIGN_START);
                if (banner_position == GTK_POS_LEFT)
                {
                    gtk_box_pack_start(GTK_BOX(box), banner_image, FALSE, FALSE, 2);
                    gtk_box_pack_start(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 2);
                    gtk_box_pack_start(GTK_BOX(box), controls, FALSE, FALSE, 2);
                }
                else
                {
                    gtk_box_pack_start(GTK_BOX(box), controls, FALSE, FALSE, 2);
                    gtk_box_pack_end(GTK_BOX(box), gtk_separator_new(GTK_ORIENTATION_VERTICAL), FALSE, FALSE, 2);
                    gtk_box_pack_end(GTK_BOX(box), banner_image, FALSE, FALSE, 2);
                }
                }
                break;

            case GTK_POS_TOP:
                gtk_box_pack_start(GTK_BOX(controls), banner_image, FALSE, FALSE, 2);
                gtk_box_pack_start(GTK_BOX(controls), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
                gtk_box_pack_start(GTK_BOX(center_vbox), controls, FALSE, FALSE, 0);
                break;

            case GTK_POS_BOTTOM:
                gtk_box_pack_end(GTK_BOX(controls), banner_image, FALSE, FALSE, 2);
                gtk_box_pack_end(GTK_BOX(controls), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), FALSE, FALSE, 2);
                gtk_box_pack_start(GTK_BOX(center_vbox), controls, FALSE, FALSE, 0);
                break;
        }
    }
    else
        gtk_box_pack_start(GTK_BOX(center_vbox), controls, FALSE, FALSE, 0);

    GtkWidget * label = gtk_label_new("");
    if (prompt == NULL)
    {
        const char * session_name = g_getenv("DESKTOP_SESSION");
        if (session_name == NULL)
            session_name = "LXDE";

        gchar *version_id = NULL;
#if GLIB_CHECK_VERSION(2, 64, 0)
        version_id = g_get_os_info(G_OS_INFO_KEY_VERSION_ID);
#endif

        if (version_id)
            prompt = g_strdup_printf(_("<b><big>Logout %s %s session?</big></b>"), session_name, version_id);
        else
            prompt = g_strdup_printf(_("<b><big>Logout %s session?</big></b>"), session_name);

        g_free(version_id);
    }
    gtk_label_set_markup(GTK_LABEL(label), prompt);
    gtk_box_pack_start(GTK_BOX(controls), label, FALSE, FALSE, 4);

    if (handler_context.shutdown_available)
    {
        GtkWidget * shutdown_button = gtk_button_new_with_mnemonic(_("Sh_utdown"));
        GtkWidget * image = gtk_image_new_from_icon_name("system-shutdown", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(shutdown_button), image);
        gtk_widget_set_halign(shutdown_button, GTK_ALIGN_START);
        gtk_widget_set_valign(shutdown_button, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(shutdown_button), "clicked", G_CALLBACK(shutdown_clicked), &handler_context);
        gtk_box_pack_start(GTK_BOX(controls), shutdown_button, FALSE, FALSE, 4);
    }

    if (handler_context.reboot_available)
    {
        GtkWidget * reboot_button = gtk_button_new_with_mnemonic(_("_Reboot"));
        GtkWidget * image = gtk_image_new_from_icon_name("gnome-session-reboot", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(reboot_button), image);
        gtk_widget_set_halign(reboot_button, GTK_ALIGN_START);
        gtk_widget_set_valign(reboot_button, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(reboot_button), "clicked", G_CALLBACK(reboot_clicked), &handler_context);
        gtk_box_pack_start(GTK_BOX(controls), reboot_button, FALSE, FALSE, 4);
    }

    if (handler_context.suspend_available && !handler_context.ltsp)
    {
        GtkWidget * suspend_button = gtk_button_new_with_mnemonic(_("_Suspend"));
        GtkWidget * image = gtk_image_new_from_icon_name("gnome-session-suspend", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(suspend_button), image);
        gtk_widget_set_halign(suspend_button, GTK_ALIGN_START);
        gtk_widget_set_valign(suspend_button, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(suspend_button), "clicked", G_CALLBACK(suspend_clicked), &handler_context);
        gtk_box_pack_start(GTK_BOX(controls), suspend_button, FALSE, FALSE, 4);
    }

    if (handler_context.hibernate_available && !handler_context.ltsp)
    {
        GtkWidget * hibernate_button = gtk_button_new_with_mnemonic(_("_Hibernate"));
        GtkWidget * image = gtk_image_new_from_icon_name("gnome-session-hibernate", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(hibernate_button), image);
        gtk_widget_set_halign(hibernate_button, GTK_ALIGN_START);
        gtk_widget_set_valign(hibernate_button, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(hibernate_button), "clicked", G_CALLBACK(hibernate_clicked), &handler_context);
        gtk_box_pack_start(GTK_BOX(controls), hibernate_button, FALSE, FALSE, 4);
    }

    if (handler_context.switch_user_available && !handler_context.ltsp)
    {
        GtkWidget * switch_user_button = gtk_button_new_with_mnemonic(_("S_witch User"));
        GtkWidget * image = gtk_image_new_from_icon_name("gnome-session-switch", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(switch_user_button), image);
        gtk_widget_set_halign(switch_user_button, GTK_ALIGN_START);
        gtk_widget_set_valign(switch_user_button, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(switch_user_button), "clicked", G_CALLBACK(switch_user_clicked), &handler_context);
        gtk_box_pack_start(GTK_BOX(controls), switch_user_button, FALSE, FALSE, 4);
    }

    if (handler_context.lock_screen && !handler_context.ltsp)
    {
        GtkWidget * lock_screen_button = gtk_button_new_with_mnemonic(_("L_ock Screen"));
        GtkWidget * image = gtk_image_new_from_icon_name("system-lock-screen", GTK_ICON_SIZE_BUTTON);
        gtk_button_set_image(GTK_BUTTON(lock_screen_button), image);
        gtk_widget_set_halign(lock_screen_button, GTK_ALIGN_START);
        gtk_widget_set_valign(lock_screen_button, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(lock_screen_button), "clicked", G_CALLBACK(lock_screen_clicked), &handler_context);
        gtk_box_pack_start(GTK_BOX(controls), lock_screen_button, FALSE, FALSE, 4);
    }

    GtkWidget * logout_button = gtk_button_new_with_mnemonic(_("_Logout"));
    GtkWidget * image = gtk_image_new_from_icon_name("system-log-out", GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(logout_button), image);
    gtk_widget_set_halign(logout_button, GTK_ALIGN_START);
    gtk_widget_set_valign(logout_button, GTK_ALIGN_CENTER);
    g_signal_connect(G_OBJECT(logout_button), "clicked", G_CALLBACK(logout_clicked), &handler_context);
    gtk_box_pack_start(GTK_BOX(controls), logout_button, FALSE, FALSE, 4);

    GtkWidget * cancel_button = gtk_button_with_mnemonic = gtk_button_new_with_mnemonic(_("_Cancel"));
    gtk_widget_set_halign(cancel_button, GTK_ALIGN_START);
    gtk_widget_set_valign(cancel_button, GTK_ALIGN_CENTER);
    g_signal_connect(G_OBJECT(cancel_button), "clicked", G_CALLBACK(cancel_clicked), NULL);
    
    GtkAccelGroup* accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(window), accel_group);
    gtk_widget_add_accelerator(cancel_button, "activate", accel_group,
        GDK_KEY_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE);
    gtk_box_pack_start(GTK_BOX(controls), cancel_button, FALSE, FALSE, 4);

    handler_context.error_label = gtk_label_new("");
    gtk_label_set_justify(GTK_LABEL(handler_context.error_label), GTK_JUSTIFY_CENTER);
    gtk_box_pack_start(GTK_BOX(controls), handler_context.error_label, FALSE, FALSE, 4);

    /* Enforce visual composition via CSS styling */
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "window, eventbox { background-color: black; }", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(window),
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_provider(gtk_widget_get_style_context(center_area),
                                   GTK_STYLE_PROVIDER(provider),
                                   GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}