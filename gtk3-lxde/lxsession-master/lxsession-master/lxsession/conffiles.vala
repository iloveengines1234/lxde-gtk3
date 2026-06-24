/* * Copyright 2012 Julien Lavergne <gilir@ubuntu.com>
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

namespace Lxsession
{
    // Fix 2: Linking explicitly to the core type layer definitions parsed in your session project
    public string get_config_path(string name);

    public class ConffilesObject : GLib.Object
    {
        public KeyFile kf;

        public string openbox_dest;
        public string qt_dest;
        public string leafpad_dest;
        public string lxterminal_dest;
        public string xscreensaver_dest;
        public string libfm_dest;
        public string cairo_dock_dest;

        public ConffilesObject(string conffiles_conf)
        {
            /* Safely allocate target platform destinations within the constructor instance scope */
            this.qt_dest = Path.build_filename(Environment.get_user_config_dir(), "Trolltech.conf");
            this.leafpad_dest = Path.build_filename(Environment.get_user_config_dir(), "leafpad", "leafpadrc");
            this.lxterminal_dest = Path.build_filename(Environment.get_user_config_dir(), "lxterminal", "lxterminal.conf");
            this.xscreensaver_dest = Path.build_filename(Environment.get_home_dir(), ".xscreensaver");
            this.libfm_dest = Path.build_filename(Environment.get_user_config_dir(), "libfm", "libfm.conf");
            this.cairo_dock_dest = Path.build_filename(Environment.get_user_config_dir(), "cairo-dock", "cairo-dock.conf");

            this.kf = new KeyFile();
            try
            {
                this.kf.load_from_file(conffiles_conf, KeyFileFlags.NONE);
            }
            catch (GLib.Error err)
            {
                warning("Failed to parse configuration mappings source file: %s", err.message);
            }

            // Sanitize desktop environment fallback assignments securely
            if (global_settings != null && global_settings.get_item_string("Session", "windows_manager", "command") == "openbox")
            {
                string session_type = global_settings.get_item_string("Session", "windows_manager", "session");
                if (session_type == "Lubuntu")
                {
                    this.openbox_dest = Path.build_filename(Environment.get_user_config_dir(), "openbox", "lubuntu-rc.xml");
                    return;
                }
            }
            this.openbox_dest = Path.build_filename(Environment.get_user_config_dir(), "openbox", "lxde-rc.xml");
        }

        public void copy_file(string source_path, string dest_path)
        {
            // Fix 3: Reject uninitialized or missing paths early to prevent parsing exceptions
            if (source_path == null || source_path.strip() == "" || dest_path == null || dest_path.strip() == "")
            {
                return;
            }

            File source_file = File.new_for_path(source_path);
            File dest_file = File.new_for_path(dest_path);
            File? dest_directory = dest_file.get_parent();

            if (!dest_file.query_exists())
            {
                // Fix 1: Validate parent directory references to prevent null pointer segmentation faults
                if (dest_directory != null && !dest_directory.query_exists())
                {
                    try
                    {
                        dest_directory.make_directory_with_parents(null);
                    }
                    catch (GLib.Error err)
                    {
                        message("Failed to generate application paths layout structure: %s", err.message);
                    }
                }

                // Verify that the source configuration profile file actually exists before deploying it
                if (!source_file.query_exists())
                {
                    warning("Source deployment template not discovered: %s", source_path);
                    return;
                }

                try
                {
                    source_file.copy(dest_file, FileCopyFlags.NONE, null, null);
                    message("Successfully synchronized configuration template: %s -> %s", source_path, dest_path);
                }
                catch (GLib.Error err)
                {
                    message("Error writing configuration block deployment: %s", err.message);
                }
            }
        }
        
        public string load_source_path(string config_type)
        {
            if (this.kf == null) return "";
            
            try
            {
                return this.kf.get_value(config_type, "source");
            }
            catch (GLib.Error err)
            {
                message("Missing configuration map index group: %s", err.message);
                return "";
            }
        }
        
        public void copy_conf(string config_type, string dest_path)
        {
            if (this.kf != null && this.kf.has_group(config_type))
            {
                string source = load_source_path(config_type);
                if (source != "")
                {
                    copy_file(source, dest_path);
                }
            }
        }
        
        public void apply()
        {
            copy_conf("Openbox", this.openbox_dest);
            copy_conf("Qt", this.qt_dest);
            copy_conf("Leafpad", this.leafpad_dest);
            copy_conf("Lxterminal", this.lxterminal_dest);
            copy_conf("XScreensaver", this.xscreensaver_dest);
            copy_conf("libfm", this.libfm_dest);
            copy_conf("cairo-dock", this.cairo_dock_dest);
        }
    }
}