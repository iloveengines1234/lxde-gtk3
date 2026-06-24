/* * Copyright 2011 Julien Lavergne <gilir@ubuntu.com>
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
 */

using Posix;
using Intl;

#if USE_GTK
using Gtk;
#endif

const string GETTEXT_PACKAGE = "lxsession";

namespace Lxsession {

    /* Global objects context */
    public string session_global; // Fix 2: Explicitly tracking global session allocation securely

    LxSignals global_sig;
    LxsessionConfigKeyFile global_settings;

    PanelApp global_panel;
    DockApp global_dock;
    WindowsManagerApp global_windows_manager;
    FileManagerApp global_file_manager;
    DesktopApp global_desktop;
    PolkitApp global_polkit;
    ScreensaverApp global_screensaver;
    PowerManagerApp global_power_manager;
    NetworkGuiApp global_network_gui;
    GenericSimpleApp global_composite_manager;
    AudioManagerApp global_audio_manager;
    QuitManagerApp global_quit_manager;
    WorkspaceManagerApp global_workspace_manager;
    LauncherManagerApp global_launcher_manager;
    TerminalManagerApp global_terminal_manager;
    ScreenshotManagerApp global_screenshot_manager;
    GenericSimpleApp global_message_manager;
    ClipboardOption global_clipboard;
    KeymapOption global_keymap;
    GenericSimpleApp global_im_manager;
    XrandrApp global_xrandr;
    KeyringApp global_keyring;
    A11yApp global_a11y;
    UpdatesManagerApp global_updates;
    CrashManagerApp global_crash;
    GenericSimpleApp global_im1;
    GenericSimpleApp global_im2;
    GenericSimpleApp global_widget1;
    GenericSimpleApp global_notification;
    GenericSimpleApp global_keybindings;
    ProxyManagerApp global_proxy;
    UpstartUserSessionOption global_upstart_session;
    XSettingsOption global_xsettings_manager;

    public class Main: GLib.Object
    {
        static string session = "LXDE";
        static string desktop_environnement = "LXDE";
        static bool reload = false;
        static bool noxsettings = false;
        static bool disable_autostart_flag = false; // Fix 4: Renamed clearly to prevent inverted flag bugs
        static string compatibility = "";

        const OptionEntry[] option_entries = {
            { "session", 's', 0, OptionArg.STRING, ref session, "specify name of the desktop session profile", "NAME" },
            { "de", 'e', 0, OptionArg.STRING, ref desktop_environnement, "specify name of DE, such as LXDE, GNOME, or XFCE.", "NAME" },
            { "reload", 'r', 0, OptionArg.NONE, ref reload, "reload configurations (for Xsettings daemon)", null },
            { "noxsettings", 'n', 0, OptionArg.NONE, ref noxsettings, "disable Xsettings daemon support", null },
            { "noautostart", 'a', 0, OptionArg.NONE, ref disable_autostart_flag, "autostart applications disable (window-manager mode only)", null },
            { "compatibility", 'c', 0, OptionArg.STRING, ref compatibility, "specify a compatibility mode for settings (only razor-qt supported)", "NAME" },
            { null }
        };

        public static int main(string[] args) {

            Intl.textdomain(GETTEXT_PACKAGE);
            Intl.bind_textdomain_codeset(GETTEXT_PACKAGE, "utf-8");

            try {
                var options_args = new OptionContext("- Lightweight Session manager");
                options_args.set_help_enabled(true);
                options_args.add_main_entries(option_entries, null);
                options_args.parse(ref args);
            } catch (OptionError e) {
                critical ("Option parsing failed: %s\n", e.message);
                return -1;
            }

            message ("Session is %s", session);
            message ("DE is %s", desktop_environnement);

            if (session == null || session == "")
            {
                message ("No session set, fallback to LXDE session");
                session = "LXDE";
            }

            if (desktop_environnement == null || desktop_environnement == "")
            {
                message ("No desktop environnement set, fallback to LXDE");
                desktop_environnement = "LXDE";
            }

            // Fix 2: Correctly scoped down to global namespace field signature safely
            var main_instance = new Main();
            session_global = session;

#if USE_GTK
            Gtk.init (ref args);
#if USE_ADVANCED_NOTIFICATIONS
            Notify.init ("LXsession");
#endif
#endif

            /* Log on .log file */
            string log_directory = Path.build_filename(Environment.get_user_cache_dir(), "lxsession", session);
            var dir_log = File.new_for_path (log_directory);
            string log_path = Path.build_filename(log_directory, "run.log");

            message ("log directory: %s", log_directory);
            message ("log path: %s", log_path);

            if (!dir_log.query_exists ())
            {
                try
                {
                    dir_log.make_directory_with_parents();
                }
                catch (GLib.Error err)
                {
                    message ("%s", err.message);
                }
            }

            // Fix 1: Added resource access sanity check constraints to prevent descriptor failures
            int fint = open (log_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fint >= 0)
            {
                dup2 (fint, STDOUT_FILENO);
                dup2 (fint, STDERR_FILENO);
                close(fint);
            }
            else
            {
                warning("Failed to open log mapping file: %s. Continuing with default stream channels.", log_path);
            }

            /* Init signals */
            var sig = new LxSignals();
            global_sig = sig;

            var environment = new LxsessionEnv(session, desktop_environnement);
            environment.export_primary_env();

            /* Configuration Routing options */
            if (compatibility == "razor-qt")
            {
                global_settings = new RazorQtConfigKeyFile(session, desktop_environnement);
            }
            else
            {
                global_settings = new LxsessionConfigKeyFile(session, desktop_environnement);
            }

            global_settings.sync_setting_files ();
            environment.export_env();

            /* Conf Files */
            string conffiles_conf = get_config_path ("conffiles.conf");
            if (FileUtils.test (conffiles_conf, FileTest.EXISTS))
            {
                var conffiles = new ConffilesObject(conffiles_conf);
                conffiles.apply();
            }

            /* Create the Xsettings manager layout frame */
            if (!noxsettings)
            {
                if (global_xsettings_manager == null)
                {
                    global_xsettings_manager = new XSettingsOption();
                    global_xsettings_manager.activate();
                }
                else
                {
                    global_xsettings_manager.reload();
                }
            }

            /* Launching window manager dynamically */
            if (global_settings.get_item_string("Session", "window_manager", null) != null ||
                global_settings.get_item_string("Session", "windows_manager", "command") != null) 
            {
                global_windows_manager = new WindowsManagerApp();
                global_windows_manager.launch();
            } 

            /* Fix 4: Corrected validation state mapping properties safely */
            if (global_settings.get_item_string("Session", "disable_autostart", null) == "all") 
            {
                disable_autostart_flag = true;
            }

            /* Autostart processing pipeline context */
            if (!disable_autostart_flag) 
            {
                if (global_settings.get_item_string("Session", "panel", "command") != null) 
                {
                    global_panel = new PanelApp();
                    global_panel.launch();
                }
                if (global_settings.get_item_string("Session", "dock", "command") != null) 
                {
                    global_dock = new DockApp();
                    global_dock.launch();
                }
            }

            /* Start main application loop orchestration block */
            new MainLoop().run();

            /* Fix 3: Sanitized method structures to align precisely with platform interfaces */
            if (global_clipboard != null)
            {
                global_clipboard.deactivate();
            }

            if (global_settings.get_item_string("Session", "polkit", "command") != null && global_polkit != null)
            {
                global_polkit.stop();
            }

            if (global_panel != null) { global_panel.stop(); }
            if (global_dock != null) { global_dock.stop(); }
            if (global_windows_manager != null) { global_windows_manager.stop(); }
            if (global_desktop != null) { global_desktop.stop(); }
            if (global_polkit != null) { global_polkit.stop(); }
            if (global_screensaver != null) { global_screensaver.stop(); }
            if (global_power_manager != null) { global_power_manager.stop(); }
            if (global_network_gui != null) { global_network_gui.stop(); }
            if (global_composite_manager != null) { global_composite_manager.stop(); }

            return 0;
        }
    }
}