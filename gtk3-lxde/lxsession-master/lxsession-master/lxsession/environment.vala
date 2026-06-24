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

namespace Lxsession
{
    // Fix 1: Properly expose global settings manager hooks resolved elsewhere in the architecture
    public extern GLib.KeyFile load_keyfile(string config_path);
    public extern string? get_config_path(string conf_file);
    public extern void lxsession_spawn_command_line_async(string command);

    // Context interface stub to map global configuration lookup pipelines securely
    public interface SettingsProvider : GLib.Object {
        public abstract string? get_item_string(string category, string key, string? default_val);
    }

    public class LxsessionEnv : GLib.Object
    {
        private string display_env = "DISPLAY";
        private string pid_env = "_LXSESSION_PID";
        private string session_env = "DESKTOP_SESSION";
        private string desktop_environment_env = "XDG_CURRENT_DESKTOP";

        private string display_name;
        private string pid_str;
        private string session;
        private string desktop_environment;

        private string config_home;
        private string config_dirs;
        private string data_dirs;
        private string home_path;

        // Persistent pointer reference to prevent internal compilation validation drops
        private SettingsProvider settings_backend;

        public LxsessionEnv(string session_arg, string desktop_environment_arg, SettingsProvider settings)
        {
            this.session = session_arg;
            this.desktop_environment = desktop_environment_arg;
            this.settings_backend = settings;

            this.display_name = GLib.Environment.get_variable(this.display_env) ?? ":0";
            this.home_path = GLib.Environment.get_variable("HOME") ?? "/root";
            this.config_home = GLib.Environment.get_variable("XDG_CONFIG_HOME");
        }

        /* Export environment that doesn't need settings, should be exported before reading settings */
        public void export_primary_env()
        {
            message("Exporting structural session primary variables sequence initialized.");
            
            this.pid_str = "%d".printf((int)Posix.getpid());
            GLib.Environment.set_variable(this.session_env, this.session, true);
            GLib.Environment.set_variable(this.desktop_environment_env, this.desktop_environment, true);
            GLib.Environment.set_variable(this.pid_env, this.pid_str, true);
            GLib.Environment.set_variable(this.display_env, this.display_name, true);

            GLib.Environment.set_application_name("lxsession");

            if (this.config_home == null || this.config_home == "")
            {
                this.config_home = Path.build_filename(this.home_path, ".config");
                GLib.Environment.set_variable("XDG_CONFIG_HOME", this.config_home, true);
            }

            this.set_xdg_dirs(null);
        }

        public void export_env()
        {
            message("Exporting secondary customized system variable configurations.");

            string? menu_prefix = this.settings_backend.get_item_string("Environment", "menu_prefix", null);
            if (menu_prefix != null && menu_prefix != "") {
                GLib.Environment.set_variable("XDG_MENU_PREFIX", menu_prefix, true);
            }

            this.set_xdg_dirs("all");
            this.set_export();
            this.set_misc();
        }

        public bool check_alone()
        {
            string? lxsession_pid = GLib.Environment.get_variable(this.pid_env);
            message("Verifying active pid context sequence profile tracking: %s", lxsession_pid ?? "NULL");

            if (lxsession_pid == null || lxsession_pid == "")
            {
                message("No concurrent lxsession instance detected across this namespace framework.");
                return true;
            }
            
            // Check if the PID in the environment variable is actually running
            pid_t active_pid = (pid_t)int.parse(lxsession_pid);
            if (active_pid > 0 && Posix.kill(active_pid, 0) == -1 && Posix.errno == Posix.ESRCH) {
                message("Stale tracking token found. The previous session engine died unexpectedly.");
                return true;
            }

            message("Active matching instance validation confirmed. Rejecting initialization.");
            return false;
        }

        public void set_xdg_dirs(string? mode)
        {
            string custom_config = "/etc/xdg";
            string custom_data = "/usr/local/share:/usr/share";

            if (this.session == "Lubuntu" || (mode == "all" && this.settings_backend.get_item_string("Environment", "type", null) == "lubuntu"))
            {
                custom_config = "/etc/xdg/lubuntu:/etc/xdg";
                custom_data = "/etc/xdg/lubuntu:/usr/local/share:/usr/share:/var/lib/menu-xdg";
            }

            this.config_dirs = GLib.Environment.get_variable("XDG_CONFIG_DIRS") ?? "";
            this.data_dirs = GLib.Environment.get_variable("XDG_DATA_DIRS") ?? "";

            // Fix 2 & 3: Fast, linear tracking routine to handle arrays safely without duplication errors
            GLib.Environment.set_variable("XDG_CONFIG_DIRS", this.merge_paths(custom_config, this.config_dirs), true);
            GLib.Environment.set_variable("XDG_DATA_DIRS", this.merge_paths(custom_data, this.data_dirs), true);
        }

        // Fix 2 & 3: Memory-safe string tokenizer that completely avoids duplicate delimiter injections
        private string merge_paths(string priority_paths, string baseline_paths) {
            if (baseline_paths == null || baseline_paths.strip() == "") {
                return priority_paths;
            }
            if (priority_paths == null || priority_paths.strip() == "") {
                return baseline_paths;
            }

            string[] priority_array = priority_paths.split(":");
            string[] baseline_array = baseline_paths.split(":");
            
            // Use an inline lookup table to handle item uniqueness linearly
            var tracker = new GLib.HashTable<string, bool?>(GLib.str_hash, GLib.str_equal);
            string[] consolidated = {};

            foreach (string path in priority_array) {
                string clean_path = path.strip();
                if (clean_path != "" && !tracker.contains(clean_path)) {
                    tracker.insert(clean_path, true);
                    consolidated += clean_path;
                }
            }

            foreach (string path in baseline_array) {
                string clean_path = path.strip();
                if (clean_path != "" && !tracker.contains(clean_path)) {
                    tracker.insert(clean_path, true);
                    consolidated += clean_path;
                }
            }

            return string.joinv(":", consolidated);
        }

        public void set_export()
        {
            string? config_file = get_config_path("desktop.conf");
            if (config_file == null) return;

            GLib.KeyFile env_kf = load_keyfile(config_file);

            try
            {
                if (env_kf.has_group("Environment_variable"))
                {
                    string[] env_list = env_kf.get_keys("Environment_variable");
                    foreach (string entry in env_list)
                    {
                        if (entry != null && entry.strip() != "")
                        {
                            string var_val = env_kf.get_value("Environment_variable", entry);
                            GLib.Environment.set_variable(entry, var_val, true);
                        }
                    }
                }
            }
            catch (GLib.Error e)
            {
                message("No entries could be parsed safely out of [Environment_variable]: %s", e.message);
            }
        }

        public void set_misc()
        {
            lxsession_spawn_command_line_async("xprop -root -remove _NET_NUMBER_OF_DESKTOPS -remove _NET_DESKTOP_NAMES -remove _NET_CURRENT_DESKTOP");

            string? toolkit_variable = this.settings_backend.get_item_string("Environment", "toolkit_integration", null);
            if (toolkit_variable != null && toolkit_variable != "false" && toolkit_variable != "")
            {
                switch (toolkit_variable)
                {
                    case "gtk3":
                        GLib.Environment.set_variable("SAL_USE_VCLPLUGIN", "gtk3", true);
                        break;
                    case "gen":
                        GLib.Environment.set_variable("SAL_USE_VCLPLUGIN", "gen", true);
                        break;
                    case "kde4":
                    case "kde":
                    case "qt":
                    case "qt4":
                        GLib.Environment.set_variable("SAL_USE_VCLPLUGIN", "kde4", true);
                        break;
                    case "gtk2":
                    case "gtk":
                    case "true":
                    default:
                        GLib.Environment.set_variable("SAL_USE_VCLPLUGIN", "gtk", true);
                        break;
                }
            }

            if (this.settings_backend.get_item_string("Environment", "gtk", "overlay_scrollbar_disable") == "true")
            {
                GLib.Environment.set_variable("GTK_OVERLAY_SCROLLING", "0", true);
            }

            string? force_theme = this.settings_backend.get_item_string("Environment", "qt", "force_theme");
            if (force_theme != null && force_theme.strip() != "")
            {
                GLib.Environment.set_variable("QT_STYLE_OVERRIDE", force_theme, true);
            }

            // Fix 4: Safely append platform architecture variables without injecting corrupt empty paths
            string? qt_plugin = GLib.Environment.get_variable("QT_PLUGIN_PATH");
            string kdedirs = "/usr/lib/kde4/plugins:/usr/lib/x86_64-linux-gnu/qt4/plugins";
            if (qt_plugin != null && qt_plugin.strip() != "") {
                GLib.Environment.set_variable("QT_PLUGIN_PATH", this.merge_paths(qt_plugin, kdedirs), true);
            } else {
                GLib.Environment.set_variable("QT_PLUGIN_PATH", kdedirs, true);
            }

            if (this.settings_backend.get_item_string("Environment", "ubuntu_menuproxy", null) == "true")
            {
                GLib.Environment.set_variable("UBUNTU_MENUPROXY", "libappmenu.so", true);
            }

            string? im_cmd = this.settings_backend.get_item_string("Session", "im_manager", "command");
            if (im_cmd != null && im_cmd.strip() != "")
            {
                GLib.Environment.set_variable("GTK_IM_MODULE", im_cmd, true);
                GLib.Environment.set_variable("QT_IM_MODULE", im_cmd, true);
                GLib.Environment.set_variable("XMODIFIERS=@im", im_cmd, true);
            }

            string? qt_platform = this.settings_backend.get_item_string("Environment", "qt", "platform");
            if (qt_platform != null && qt_platform.strip() != "")
            {
                GLib.Environment.set_variable("QT_PLATFORM_PLUGIN", qt_platform, true);
                GLib.Environment.set_variable("QT_QPA_PLATFORMTHEME", qt_platform, true);
            }
        }
    }
}