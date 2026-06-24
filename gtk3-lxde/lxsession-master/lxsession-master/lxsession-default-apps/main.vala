/* Copyright 2012 Julien Lavergne <gilir@ubuntu.com>
 * Ported to GTK 3
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

using Gtk;
using GLib;
using Posix;

const string GETTEXT_PACKAGE = "lxsession";

namespace LDefaultApps
{
    public class UpdateWindows : Gtk.Window
    {
        public Pid pid;
        public KeyFile kf;

        public UpdateWindows ()
        {
            this.title = _("Update lxsession database");
            this.window_position = Gtk.WindowPosition.CENTER;

            try
            {
                this.icon = IconTheme.get_default ().load_icon ("preferences-desktop", 48, 0);
            }
            catch (Error e)
            {
                message ("Could not load application icon: %s\n", e.message);
            }

            this.set_default_size (300, 70);

            // Simple content label
            var label = new Gtk.Label (_("The database is updating, please wait"));
            label.margin = 12;
            label.halign = Gtk.Align.CENTER;
            label.valign = Gtk.Align.CENTER;
            this.add (label);

            kf = get_keyfile ();
            update_database ();
        }

        public void update_database ()
        {
            try
            {
                string[] command = "lxsession-db -m write".split_set (" ", 0);

                Process.spawn_async (
                    null,
                    command,
                    null,
                    SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD,
                    null,
                    out pid
                );

                ChildWatch.add (pid, callback_pid);
            }
            catch (SpawnError err)
            {
                warning ("Error updating the database: %s\n", err.message);
            }
        }

        private void callback_pid (Pid pid, int status)
        {
            Process.close_pid (pid);

            var win = new MainWindows (kf);
            win.show_all ();

            // GTK3‑friendly: just hide this window, let main loop continue
            this.hide ();
        }

        public KeyFile get_keyfile ()
        {
            string config_path_directory =
                Path.build_filename (Environment.get_user_config_dir (), "lxsession-default-apps");

            KeyFile kf = load_key_conf (config_path_directory, "settings.conf");
            return kf;
        }
    }

    public class MainWindows : Gtk.Window
    {
        public MainWindows (KeyFile kf)
        {
            this.title = _("LXSession configuration");
            this.window_position = Gtk.WindowPosition.CENTER;

            try
            {
                this.icon = IconTheme.get_default ().load_icon ("preferences-desktop", 48, 0);
            }
            catch (Error e)
            {
                message ("Could not load application icon: %s\n", e.message);
            }

            this.set_default_size (600, 400);
            this.destroy.connect (Gtk.main_quit);

            // Load UI from GtkBuilder
            var builder = new Gtk.Builder ();

            try
            {
                string builder_file_path = Path.build_filename ("data", "ui", "lxsession-default-apps.ui");
                var builder_file = File.new_for_path (builder_file_path);

                if (builder_file.query_exists ())
                {
                    builder.add_from_file (builder_file_path);
                }
                else
                {
                    // Fallback to system prefix
                    builder_file_path = Path.build_filename (
                        "/usr", "share", "lxsession", "ui", "lxsession-default-apps.ui"
                    );
                    builder.add_from_file (builder_file_path);
                }
            }
            catch (GLib.Error e)
            {
                critical ("Could not load UI: %s\n", e.message);
            }

            builder.connect_signals (null);

            // main_vbox is now a GtkBox in the GTK3 .ui file
            var main_vbox = builder.get_object ("main_vbox") as Gtk.Box;
            if (main_vbox == null)
            {
                critical ("Could not find main_vbox in UI file\n");
            }
            else
            {
                main_vbox.margin = 6;
                this.add (main_vbox);
            }

            // Optional: override icon if available
            try
            {
                this.icon = IconTheme.get_default ().load_icon ("xfwm4", 48, 0);
            }
            catch (Error e)
            {
                message ("Could not load application icon: %s\n", e.message);
            }

            var dbus_backend = new DbusBackend ("session");

            /* Autostart list */
            manual_autostart_init (builder);
            autostart_core_applications (builder, dbus_backend);

            /* Common help strings */
            string manual_setting_help =
                _("Manual Settings: Manually sets the command (you need to restart lxsession-default-apps to see the change)\n");
            string session_string_help = _("Session : specify the session\n");
            string extra_string_help = _("Extra: Add an extra parameter to the launch option\n");
            string mime_association_help =
                _("Mime association: Automatically associates mime types to this application?\n");
            string mime_available_help =
                _("Available applications : Applications of this type available on your repositories\n");
            string handle_desktop_help =
                _("Handle Desktop: Draw the desktop using the file manager?\n");
            string autostart_help = _("Autostart the application?\n");
            string debian_default_help =
                _("Set default program for Debian system (using update-alternatives, need root password)\n");

            /* Windows manager */
            string windows_manager_help_message =
                _("Windows manager draws and manage the windows. \nYou can choose openbox, openbox-custom (for a custom openbox configuration, see \"More\"), kwin, compiz ...");
            string[] windows_manager_more = { "session", "extras" };
            string windows_manager_more_help_message =
                session_string_help + extra_string_help;
            init_application (
                builder, kf, dbus_backend,
                "windows_manager", "",
                windows_manager_help_message,
                windows_manager_more,
                windows_manager_more_help_message,
                null
            );

            /* Panel */
            string panel_help_message =
                _("Panel is the component usually at the bottom of the screen which manages a list of opened windows, shortcuts, notification area ...");
            string[] panel_more = { "session" };
            string panel_more_help_message = session_string_help;
            init_application (
                builder, kf, dbus_backend,
                "panel", "",
                panel_help_message,
                panel_more,
                panel_more_help_message,
                null
            );

            /* Dock */
            string dock_help_message =
                _("Dock is a second panel. It's used to launch a different program to handle a second type of panel.");
            string[] dock_more = { "session" };
            string dock_more_help_message = session_string_help;
            init_application (
                builder, kf, dbus_backend,
                "dock", "",
                dock_help_message,
                dock_more,
                dock_more_help_message,
                null
            );

            /* File manager */
            string file_manager_help_message =
                _("File manager is the component which open the files.\nSee \"More\" to add options to handle the desktop, or opening files ");
            string[] file_manager_more = {
                "combobox_manual", "session", "extra",
                "handle_desktop", "mime_association", "mime_available"
            };
            string file_manager_more_help_message =
                manual_setting_help + session_string_help + extra_string_help +
                handle_desktop_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "file_manager", "",
                file_manager_help_message,
                file_manager_more,
                file_manager_more_help_message,
                null
            );

            /* Composite manager */
            string composite_manager_help_message =
                _("Composite manager enables graphics effects, like transparency and shadows, if the windows manager doesn't handle it. \nExample: compton");
            string[] composite_manager_more = { "" };
            string composite_manager_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "composite_manager", "",
                composite_manager_help_message,
                composite_manager_more,
                composite_manager_more_help_message,
                null
            );

            /* Desktop manager */
            string desktop_manager_help_message =
                _("Desktop manager draws the desktop and manages the icons inside it.\nYou can manage it with the file manager by setting \"filemanager\"");
            string[] desktop_manager_more = { "wallpaper", "handle_desktop" };
            string desktop_manager_more_help_message =
                _("Wallpaper: Set an image path to draw the wallpaper");
            init_application (
                builder, kf, dbus_backend,
                "desktop_manager", "",
                desktop_manager_help_message,
                desktop_manager_more,
                desktop_manager_more_help_message,
                null
            );

            /* Screensaver */
            string screensaver_help_message =
                _("Screensaver is a program which displays animations when your computer is idle");
            string[] screensaver_more = { "" };
            string screensaver_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "screensaver", "",
                screensaver_help_message,
                screensaver_more,
                screensaver_more_help_message,
                null
            );

            /* Power manager */
            string power_manager_help_message =
                _("Power Manager helps you to reduce the usage of batteries. You probably don't need one if you have a desktop computer.\nAuto option will set it automatically, depending of the laptop mode option.");
            string[] power_manager_more = { "" };
            string power_manager_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "power_manager", "",
                power_manager_help_message,
                power_manager_more,
                power_manager_more_help_message,
                null
            );

            /* Polkit */
            string polkit_help_message =
                _("Polkit agent provides authorisations to use some actions, like suspend, hibernate, using Consolekit ... It's not advised to make it blank.");
            string[] polkit_more = { "" };
            string polkit_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "polkit", "",
                polkit_help_message,
                polkit_more,
                polkit_more_help_message,
                null
            );

            /* Network GUI */
            string network_gui_help_message =
                _("Set an utility to manager connections, such as nm-applet");
            string[] network_gui_more = { "" };
            string network_gui_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "network_gui", "",
                network_gui_help_message,
                network_gui_more,
                network_gui_more_help_message,
                null
            );

            /* IM1 */
            string im1_help_message =
                _("Use a communication software (an IRC client, an IM client ...)");
            string[] im1_more = {
                "combobox_manual", "autostart",
                "mime_association", "mime_available"
            };
            string im1_more_help_message =
                manual_setting_help + autostart_help +
                mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "im1", "",
                im1_help_message,
                im1_more,
                im1_more_help_message,
                "im"
            );

            /* IM2 */
            string im2_help_message =
                _("Use another communication software (an IRC client, an IM client ...)");
            string[] im2_more = {
                "combobox_manual", "autostart",
                "mime_association", "mime_available"
            };
            string im2_more_help_message =
                manual_setting_help + autostart_help +
                mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "im2", "",
                im2_help_message,
                im2_more,
                im2_more_help_message,
                "im"
            );

            /* Terminal manager */
            string terminal_manager_help_message =
                _("Terminal by default to launch command line.");
            string[] terminal_manager_more = {
                "combobox_manual", "debian_default",
                "mime_association", "mime_available"
            };
            string terminal_manager_more_help_message =
                manual_setting_help + debian_default_help +
                mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "terminal_manager", "",
                terminal_manager_help_message,
                terminal_manager_more,
                terminal_manager_more_help_message,
                null
            );

            /* Web browser */
            string webbrowser_help_message =
                _("Application to go to Internet, Google, Facebook, debian.org ...");
            string[] webbrowser_more = {
                "combobox_manual", "debian_default",
                "mime_association", "mime_available"
            };
            string webbrowser_more_help_message =
                manual_setting_help + debian_default_help +
                mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "webbrowser", "",
                webbrowser_help_message,
                webbrowser_more,
                webbrowser_more_help_message,
                null
            );

            /* Email */
            string email_help_message = _("Application to send mails");
            string[] email_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string email_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "email", "",
                email_help_message,
                email_more,
                email_more_help_message,
                null
            );

            /* Widget */
            string widget_help_message =
                _("Utility to launch gadgets, like conky, screenlets ...");
            string[] widget_more = { "autostart" };
            string widget_more_help_message = autostart_help;
            init_application (
                builder, kf, dbus_backend,
                "widget1", "",
                widget_help_message,
                widget_more,
                widget_more_help_message,
                "widget"
            );

            /* Launcher manager */
            string launcher_manager_help_message =
                _("Utility to launch application, like synapse, kupfer ... \nFor using lxpanel or lxde default utility, use \"lxpanelctl\" ");
            string[] launcher_manager_more = { "autostart" };
            string launcher_manager_more_help_message = autostart_help;
            init_application (
                builder, kf, dbus_backend,
                "launcher_manager", "",
                launcher_manager_help_message,
                launcher_manager_more,
                launcher_manager_more_help_message,
                null
            );

            /* Screenshot manager */
            string screenshot_manager_help_message =
                _("Application for taking screeshot of your desktop, like scrot ...");
            string[] screenshot_manager_more = { "" };
            string screenshot_manager_more_help_message = autostart_help;
            init_application (
                builder, kf, dbus_backend,
                "screenshot_manager", "",
                screenshot_manager_help_message,
                screenshot_manager_more,
                screenshot_manager_more_help_message,
                null
            );

            /* PDF reader */
            string pdf_reader_help_message =
                _("Viewer for PDF, like evince");
            string[] pdf_reader_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string pdf_reader_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "pdf_reader", "",
                pdf_reader_help_message,
                pdf_reader_more,
                pdf_reader_more_help_message,
                null
            );

            /* Video player */
            string video_player_help_message = _("Video application");
            string[] video_player_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string video_player_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "video_player", "",
                video_player_help_message,
                video_player_more,
                video_player_more_help_message,
                null
            );

            /* Audio player */
            string audio_player_help_message = _("Audio application");
            string[] audio_player_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string audio_player_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "audio_player", "",
                audio_player_help_message,
                audio_player_more,
                audio_player_more_help_message,
                null
            );

            /* Image display */
            string image_display_help_message = _("Application to display images");
            string[] image_display_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string image_display_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "image_display", "",
                image_display_help_message,
                image_display_more,
                image_display_more_help_message,
                "image_display"
            );

            /* Text editor */
            string text_editor_help_message = _("Application to edit text");
            string[] text_editor_more = {
                "combobox_manual", "mime_association",
                "mime_available", "debian_default"
            };
            string text_editor_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "text_editor", "",
                text_editor_help_message,
                text_editor_more,
                text_editor_more_help_message,
                "text_editor"
            );

            /* Archive */
            string archive_help_message =
                _("Application to create archives, like file-roller");
            string[] archive_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string archive_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "archive", "",
                archive_help_message,
                archive_more,
                archive_more_help_message,
                null
            );

            /* Charmap */
            string charmap_help_message = _("Charmap application");
            string[] charmap_more = { "" };
            string charmap_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "charmap", "",
                charmap_help_message,
                charmap_more,
                charmap_more_help_message,
                null
            );

            /* Calculator */
            string calculator_help_message = _("Calculator application");
            string[] calculator_more = { "" };
            string calculator_more_help_message = "";
            init_application (
                builder, kf, dbus_backend,
                "calculator", "",
                calculator_help_message,
                calculator_more,
                calculator_more_help_message,
                null
            );

            /* Spreadsheet */
            string spreadsheet_help_message =
                _("Application to create spreedsheet, like gnumeric");
            string[] spreadsheet_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string spreadsheet_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "spreadsheet", "",
                spreadsheet_help_message,
                spreadsheet_more,
                spreadsheet_more_help_message,
                null
            );

            /* Bittorrent */
            string bittorent_help_message =
                _("Application to manage bittorent, like transmission");
            string[] bittorent_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string bittorent_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "bittorent", "",
                bittorent_help_message,
                bittorent_more,
                bittorent_more_help_message,
                null
            );

            /* Document */
            string document_help_message =
                _("Application to manage office text, like abiword");
            string[] document_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string document_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "document", "",
                document_help_message,
                document_more,
                document_more_help_message,
                null
            );

            /* Webcam */
            string webcam_help_message = _("Application to manage webcam");
            string[] webcam_more = { "" };
            string webcam_more_help_message = manual_setting_help;
            init_application (
                builder, kf, dbus_backend,
                "webcam", "",
                webcam_help_message,
                webcam_more,
                webcam_more_help_message,
                null
            );

            /* Burn */
            string burn_help_message =
                _("Application to manage burning CD/DVD utilty ");
            string[] burn_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string burn_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "burn", "",
                burn_help_message,
                burn_more,
                burn_more_help_message,
                null
            );

            /* Notes */
            string notes_help_message =
                _("Application to manage notes utility");
            string[] notes_more = { "" };
            string notes_more_help_message = manual_setting_help;
            init_application (
                builder, kf, dbus_backend,
                "notes", "",
                notes_help_message,
                notes_more,
                notes_more_help_message,
                null
            );

            /* Disk utility */
            string disk_utility_help_message =
                _("Application to manage disks");
            string[] disk_utility_more = { "" };
            string disk_utility_more_help_message = manual_setting_help;
            init_application (
                builder, kf, dbus_backend,
                "disk_utility", "",
                disk_utility_help_message,
                disk_utility_more,
                disk_utility_more_help_message,
                null
            );

            /* Tasks */
            string tasks_help_message =
                _("Application to monitor tasks running on your system");
            string[] tasks_more = {
                "combobox_manual", "mime_association", "mime_available"
            };
            string tasks_more_help_message =
                manual_setting_help + mime_association_help + mime_available_help;
            init_application_combobox (
                builder, kf, dbus_backend,
                "tasks", "",
                tasks_help_message,
                tasks_more,
                tasks_more_help_message,
                null
            );

            /* Lock manager */
            string lock_manager_help_message =
                _("Application to lock your screen");
            string[] lock_manager_more = { "" };
            string lock_manager_more_help_message = manual_setting_help;
            init_application (
                builder, kf, dbus_backend,
                "lock_manager", "",
                lock_manager_help_message,
                lock_manager_more,
                lock_manager_more_help_message,
                null
            );

            /* Audio manager */
            string audio_manager_help_message =
                _("Managing your audio configuration");
            string[] audio_manager_more = { "" };
            string audio_manager_more_help_message = manual_setting_help;
            init_application (
                builder, kf, dbus_backend,
                "audio_manager", "",
                audio_manager_help_message,
                audio_manager_more,
                audio_manager_more_help_message,
                null
            );
        }
    }

    public static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (GETTEXT_PACKAGE, "/usr/share/locale");
        Intl.textdomain (GETTEXT_PACKAGE);

        Gtk.init (ref args);

        var update = new UpdateWindows ();
        update.show_all ();

        Gtk.main ();
        return 0;
    }
}
