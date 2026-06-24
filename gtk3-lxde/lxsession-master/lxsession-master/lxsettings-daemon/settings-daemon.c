/*
 * lxde-settings.c - XSettings daemon of LXDE
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
 * ported by iloveengines1234 to gtk3
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>

#include "xevent.h"
#include "xsettings-manager.h"
#include "xutils.h"

#include <X11/XKBlib.h>

static XSettingsManager **managers = NULL;

/* FORWARDS */
gboolean settings_daemon_start(GKeyFile* kf);
void settings_manager_selection_clear(XEvent* evt);
void settings_daemon_reload(GKeyFile* kf);
/* End FORWARDS */

static void terminate_cb(void *data)
{
    gboolean *terminated = data;

    if (*terminated)
        return;

    *terminated = TRUE;
    exit(0);
}

static void merge_xrdb(const char* content, int len)
{
    gchar* argv[] = { "xrdb", "-merge", "-", NULL };
    gint exit_status = 0;
    GError *error = NULL;

    /* Securely route contents to avoid leaks or uninitialized strndup boundaries */
    if (!g_spawn_sync(NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
                      NULL, NULL, (gchar*)content, NULL, &exit_status, &error))
    {
        g_warning("Failed to invoke xrdb theme subsystem: %s", error->message);
        g_error_free(error);
    }
}

#define DEFAULT_PTR_MAP_SIZE 128
static void set_left_handed_mouse(gboolean mouse_left_handed)
{
    unsigned char *buttons;
    gint n_buttons, i;
    gint idx_1 = 0, idx_3 = 1;

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

    if ((mouse_left_handed && idx_1 < idx_3) ||
        (!mouse_left_handed && idx_1 > idx_3))
    {
        buttons[idx_1] = ((n_buttons < 3) ? 2 : 3);
        buttons[idx_3] = 1;
        XSetPointerMapping(dpy, buttons, n_buttons);
    }
}

static void configure_input(GKeyFile* kf)
{
    XKeyboardControl values;
    GError *error = NULL;

    int accel_factor, accel_threshold, delay, interval;
    gboolean left_handed, beep;

    accel_factor = g_key_file_get_integer(kf, "Mouse", "AccFactor", &error);
    if (error) { accel_factor = 0; g_clear_error(&error); }

    accel_threshold = g_key_file_get_integer(kf, "Mouse", "AccThreshold", &error);
    if (error) { accel_threshold = 0; g_clear_error(&error); }

    if (accel_factor || accel_threshold)
    {
        XChangePointerControl(dpy, accel_factor != 0, accel_threshold != 0,
                              accel_factor, 10, accel_threshold);
    }

    left_handed = g_key_file_get_boolean(kf, "Mouse", "LeftHanded", &error);
    if (error) { left_handed = FALSE; g_clear_error(&error); }
    set_left_handed_mouse(left_handed);

    if (XkbGetAutoRepeatRate(dpy, XkbUseCoreKbd, (unsigned int*)&delay, (unsigned int*)&interval))
    {
        int val;
        val = g_key_file_get_integer(kf, "Keyboard", "Delay", &error);
        if (!error && val > 0) delay = val;
        g_clear_error(&error);

        val = g_key_file_get_integer(kf, "Keyboard", "Interval", &error);
        if (!error && val > 0) interval = val;
        g_clear_error(&error);

        if (interval > 0)
        {
            XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, delay, interval);
        }
    }

    beep = g_key_file_get_boolean(kf, "Keyboard", "Beep", &error);
    if (error) { beep = TRUE; g_clear_error(&error); }
    values.bell_percent = beep ? -1 : 0;
    XChangeKeyboardControl(dpy, KBBellPercent, &values);
}

static void load_settings(GKeyFile* kf)
{
    GString* buf;
    char* str;
    int val;
    int i;
    const char group[] = "GTK";
    char** keys, **key;
    GError *error = NULL;

    str = g_key_file_get_string(kf, group, "sGtk/CursorThemeName", &error);
    g_clear_error(&error);
    
    val = g_key_file_get_integer(kf, group, "iGtk/CursorThemeSize", &error);
    g_clear_error(&error);

    if (str || val > 0)
    {
        buf = g_string_sized_new(100);
        if (str)
        {
            if (*str)
                g_string_append_printf(buf, "Xcursor.theme:%s\n", str);
            g_free(str);
        }
        g_string_append(buf, "Xcursor.theme_core:true\n");
        if (val > 0)
            g_string_append_printf(buf, "Xcursor.size:%d\n", val);
        merge_xrdb(buf->str, buf->len);
        g_string_free(buf, TRUE);
    }

    configure_input(kf);

    if ((keys = g_key_file_get_keys(kf, group, NULL, NULL)) == NULL) 
        return;

    for (key = keys; *key; ++key)
    {
        const char* name = *key + 1;

        switch (**key)
        {
            case 's':
            {
                str = g_key_file_get_string(kf, group, *key, NULL);
                if (str)
                {
                    for (i = 0; managers[i]; ++i)
                        xsettings_manager_set_string(managers[i], name, str);
                    g_free(str);
                }
                else
                {
                    for (i = 0; managers[i]; ++i)
                        xsettings_manager_delete_setting(managers[i], name);
                }
                break;
            }
            case 'i':
            {
                val = g_key_file_get_integer(kf, group, *key, NULL);
                for (i = 0; managers[i]; ++i)
                    xsettings_manager_set_int(managers[i], name, val);
                break;
            }
            case 'c':
            {
                gsize len = 0;
                int* vals = g_key_file_get_integer_list(kf, group, *key, &len, NULL);
                if (vals && len >= 3)
                {
                    XSettingsColor color;
                    color.red = (gushort)vals[0];
                    color.green = (gushort)vals[1];
                    color.blue = (gushort)vals[2];
                    color.alpha = (gushort)(len > 3 ? vals[3] : 65535);
                    for (i = 0; managers[i]; ++i)
                        xsettings_manager_set_color(managers[i], name, &color);
                }
                else
                {
                    for (i = 0; managers[i]; ++i)
                        xsettings_manager_delete_setting(managers[i], name);
                }
                g_free(vals);
                break;
            }
        }
    }
    g_strfreev(keys);

    for (i = 0; managers[i]; ++i)
        xsettings_manager_notify(managers[i]);
}

static gboolean create_xsettings_managers(void)
{
    int n_screens = ScreenCount(dpy);
    int i;
    static gboolean terminated = FALSE;

    if (xsettings_manager_check_running(dpy, n_screens))
    {
        g_critical("Another active XSettings manager runtime is executing on this display. Exiting.");
        return FALSE;
    }

    managers = g_new(XSettingsManager *, n_screens + 1);
    for (i = 0; i < n_screens; ++i)
    {
        managers[i] = xsettings_manager_new(dpy, i, terminate_cb, &terminated);
        if (!managers[i])
        {
            g_warning("Could not instantiate xsettings hardware engine for layout screen %d!", i);
            return FALSE;
        }
        XSelectInput(dpy, RootWindow(dpy, i), SubstructureNotifyMask | PropertyChangeMask);
    }
    managers[i] = NULL;

    return TRUE;
}

gboolean settings_daemon_start(GKeyFile* kf)
{
    if (G_UNLIKELY(!xevent_init()))
        return FALSE;

    if (!create_xsettings_managers())
        return FALSE;

    load_settings(kf);
    XSync(dpy, FALSE);

    return TRUE;
}

void settings_manager_selection_clear(XEvent* evt)
{
    XSettingsManager** mgr;
    for (mgr = managers; *mgr; ++mgr)
    {
        if (xsettings_manager_get_window(*mgr) == evt->xany.window)
            xsettings_manager_process_event(*mgr, evt);
    }
}

void settings_daemon_reload(GKeyFile* kf)
{
    if (kf)
    {
        load_settings(kf);
    }
}