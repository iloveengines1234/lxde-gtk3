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

    /* Global application pointers initialized inside standard framework boundaries */
    public LxSignals global_sig;
    
    // Fix 1: Abstracted configuration assignment type to resolve polymorphic compilation errors
    public LxsessionConfig global_settings;

    public PanelApp global_panel;
    public DockApp global_dock;
    public WindowsManagerApp global_windows_manager;
    public FileManagerApp global_file_manager;
    public DesktopApp global_desktop;
    public PolkitApp global_polkit;
    public ScreensaverApp global_screensaver;
    public PowerManagerApp global_power_manager;
    public NetworkGuiApp global_network_gui;
    public GenericSimpleApp global_composite_manager;
    public AudioManagerApp global_audio_manager;
    public QuitManagerApp global_quit_manager;
    public WorkspaceManagerApp global_workspace_manager;
    public LauncherManagerApp global_launcher_manager;
    public TerminalManagerApp global_terminal_manager;
    public ScreenshotManagerApp global_screenshot_manager;
    public GenericSimpleApp global_message_manager;
    public ClipboardOption global_clipboard;
    public KeymapOption global_keymap;
    public GenericSimpleApp global_im_manager;
    public XrandrApp global_xrandr;
    public KeyringApp global_keyring;
    public A11yApp global_a11y;
    public UpdatesManagerApp global_updates;
    public CrashManagerApp global_crash;
    public GenericSimpleApp global_im1;
    public GenericSimpleApp global_im2;
    public GenericSimpleApp global_widget1;
    public GenericSimpleApp global_notification;
    public GenericSimpleApp global_keybindings;
    public ProxyManagerApp global_proxy;
    public UpstartUserSessionOption global_upstart_session;
    public XSettingsOption global_xsettings_manager;

    // Retain main loop scope access for signal interruption cleanup routines
    private static MainLoop main_loop_instance;

    public class Main: GLib.Object
    {
        static string session = "LXDE";
        static string desktop_environnement = "LXDE";
        static bool reload = false;
        static bool noxsettings = false;
        static bool autostart = false;
        static string compatibility = "";

        const OptionEntry[] option_entries = {
            { "session", 's', 0, OptionArg.STRING, ref session, "specify name of the desktop session profile", "NAME" },
            { "de", 'e', 0, OptionArg.STRING, ref desktop_environnement, "specify name of DE, such as LXDE, GNOME, or XFCE.", "NAME" },
            { "reload", 'r', 0, OptionArg.NONE, ref reload, "reload configurations (for Xsettings daemon)", null },
            { "noxsettings", 'n', 0, OptionArg.NONE, ref noxsettings, "disable Xsettings daemon support", null },
            { "noautostart", 'a', 0, OptionArg.NONE, ref autostart, "autostart applications disable (window-manager mode only)", null },
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

            message ("Session profile initialized: %s", session);
            message ("Desktop Environment target: %s", desktop_environnement);

            if (session == null || session.strip() == "") { session = "LXDE"; }
            if (desktop_environnement == null || desktop_environnement.strip() == "") { desktop_environnement = "LXDE"; }

            session_global = session;

#if USE_GTK
            Gtk.init (ref args);
#if USE_ADVANCED_NOTIFICATIONS
            Notify.init ("LXsession");
#endif
#endif

            /* Secure and generate diagnostic cache paths asynchronously */
            string log_directory = Path.build_filename(Environment.get_user_cache_dir(), "lxsession", session);
            var dir_log = File.new_for_path (log_directory);
            string log_path = Path.build_filename(log_directory, "run.log");

            if (!dir_log.query_exists ())
            {
                try
                {
                    dir_log.make_directory_with_parents();
                }
                catch (GLib.Error err)
                {
                    warning ("Could not provision cache runtime locations safely: %s", err.message);
                }
            }

            // Fix 2: Buffered, production standard stream redirection mechanics
            int fint = open(log_path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
            if (fint != -1) {
                dup2 (fint, STDOUT_FILENO);
                dup2 (fint, STDERR_FILENO);
                close(fint);
            }

            var sig = new LxSignals();
            global_sig = sig;

            var environment = new LxsessionEnv(session, desktop_environnement);
            environment.export_primary_env();

            if (compatibility == "razor-qt")
            {
                global_settings = new RazorQtConfigKeyFile(session, desktop_environnement);
            }
            else
            {
                global_settings = new LxsessionConfigKeyFile(session, desktop_environnement);
            }

            global_settings.sync_setting_files();
            environment.export_env();

            string conffiles_conf = get_config_path ("conffiles.conf");
            if (FileUtils.test (conffiles_conf, FileTest.EXISTS))
            {
                var conffiles = new ConffilesObject(conffiles_conf);
                conffiles.apply();
            }

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

            if (global_settings.get_item_string("Session", "window_manager", null) != null ||
                global_settings.get_item_string("Session", "windows_manager", "command") != null) 
            {
                global_windows_manager = new WindowsManagerApp();
                global_windows_manager.launch();
            } 

            if (global_settings.get_item_string("Session", "disable_autostart", null) == "all") 
            {
                autostart = true;
            }

            if (!autostart) 
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

            main_loop_instance = new MainLoop();

            // Fix 3: UNIX OS Interrupt signals captured inside the main event pump context loop
            GLib.Unix.signal_add(ProcessSignal.SIGTERM, on_shutdown_signal_received);
            GLib.Unix.signal_add(ProcessSignal.SIGINT, on_shutdown_signal_received);

            /* Start main blocking execution stream tracking framework loop */
            main_loop_instance.run();

            // Fix 4: Unified structural cleanup triggered reliably during process shutdown lifecycle transitions
            execute_clean_teardown();

            return 0;
        }

        private static bool on_shutdown_signal_received()
        {
            message("%s", "Termination interrupt caught by event listener. Initiating session teardown process...");
            if (main_loop_instance.is_running())
            {
                main_loop_instance.quit();
            }
            return GLib.Source.REMOVE;
        }

        private static void execute_clean_teardown()
        {
            // Fix 5: Swapped identifier naming convention call mechanics matching modern options definitions
            if (global_clipboard != null)
            {
                global_clipboard.deactivate();
            }

            if (global_settings.get_item_string("Session", "polkit", "command") != null && global_polkit != null)
            {
                global_polkit.deactivate();
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
            
            message("%s", "Session manager application closed cleanly.");
        }
    }
}