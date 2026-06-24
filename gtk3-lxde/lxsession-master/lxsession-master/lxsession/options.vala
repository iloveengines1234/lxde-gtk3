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

/* TODO Replace other utility in the start script */

namespace Lxsession
{
    public class Option : GLib.Object
    {
        public string command { get; protected set; default = ""; }

        public Option (LxsessionConfig config)
        {
            // Base constructor logic
        }

        // Fix 1: Declared as virtual to allow correct polymorphic overrides across children
        public virtual void activate()
        {
            // Safeguard against uninitialized, whitespace, or empty strings
            if (this.command == null || this.command.strip() == "")
            {
                return;
            }

            message ("Options - Launch command: %s", this.command);
            lxsession_spawn_command_line_async(this.command);
        }
    }

    public class KeymapOption : Option
    {
        public KeymapOption (LxsessionConfig config)
        {
            base (config);
            if (config.get_item_string("Keymap", "mode", null) == "user")
            {
                this.command = create_user_mode_command(config);
            }
        }

        public string create_user_mode_command(LxsessionConfig config)
        {
            var builder = new StringBuilder ("setxkbmap");

            string model = config.get_item_string("Keymap", "model", null);
            if (model != null && model.strip() != "")
            {
                // Fix 2: Wrap inputs safely or strictly token-check to prevent argument shell injection
                builder.append_printf(" -model '%s'", model.replace("'", ""));
            }

            string layout = config.get_item_string("Keymap", "layout", null);
            if (layout != null && layout.strip() != "")
            {
                builder.append_printf(" -layout '%s'", layout.replace("'", ""));
            }

            string variant = config.get_item_string("Keymap", "variant", null);
            if (variant != null && variant.strip() != "")
            {
                message ("Show keymap variant : %s", variant);
                builder.append_printf(" -variant '%s'", variant.replace("'", ""));
            }

            string options = config.get_item_string("Keymap", "options", null);
            if (options != null && options.strip() != "")
            {
                message ("Show keymap options : %s", options);
                builder.append_printf(" -options '%s'", options.replace("'", ""));
            }

            string final_command = builder.str;
            message ("Keymap options - return user command %s", final_command);
            return final_command;
        }
    }

    public class ClipboardOption : Option
    {
        public ClipboardOption (LxsessionConfig config)
        {
            base (config);
            string clipboard_backend = config.get_item_string("Session", "clipboard", "command");
            
            if (clipboard_backend == "lxclipboard")
            {
#if BUILDIN_CLIPBOARD
                message("%s", "Initializing built-in session framework clipboard daemon thread context...");
                clipboard_start ();
#else
                message("%s", "Registering fallback standalone lxclipboard executor interface mapping...");
                this.command = "lxclipboard";
#endif
            }
        }

        // Renamed method to correct english spelling standard frameworks
        public void deactivate()
        {
#if BUILDIN_CLIPBOARD
            clipboard_stop ();
#endif
        }
    }

    public class UpstartUserSessionOption : Option
    {
        public UpstartUserSessionOption (LxsessionConfig config)
        {
            base (config);
            if (config.get_item_string("Session", "upstart_user_session", null) == "true")
            {
                // Modern systemd-user service trigger path fallback string can be swapped here if required
                this.command = "init --user";
            }
        }

        // Fix 3: Implemented proper override mechanism instead of unsafe hiding
        public override void activate()
        {
            if (this.command != null && this.command != "")
            {
                lxsession_spawn_command_line_async(this.command);
            }
        }
    }

    public class XSettingsOption : GLib.Object
    {
        public string command { get; private set; default = ""; }

        public XSettingsOption ()
        {
            base();
        }

        // Fix 4: Removed broken 'new' keyword modifier as this class inherits directly from GLib.Object
        public void activate ()
        {
            this.command = global_settings.get_item_string("Session", "xsettings_manager", "command");

            switch (this.command)
            {
                case null:
                case "":
                case " ":
                    break;
                case "build-in":
                    message("%s", "Activating internal configuration daemon settings loop listener structures..."); 
                    settings_daemon_start(load_keyfile (get_config_path ("desktop.conf")));
                    break;
                case "gnome":
                    lxsession_spawn_command_line_async("gnome-settings-daemon");
                    break;
                case "xfce":
                    lxsession_spawn_command_line_async("xfsettingsd");
                    break;
                default:
                    lxsession_spawn_command_line_async(this.command);
                    break;
            }
        }

        public void reload ()
        {
            this.command = global_settings.get_item_string("Session", "xsettings_manager", "command");

            switch (this.command)
            {
                case "build-in":
                    message("%s", "Reloading internal environment setting configurations maps..."); 
                    settings_daemon_reload(load_keyfile (get_config_path ("desktop.conf")));
                    break;
                default:
                    message("%s", "Defaulting to fallback cold redirection restart layout framework setup..."); 
                    this.activate();
                    break;
            }
        }
    }
}