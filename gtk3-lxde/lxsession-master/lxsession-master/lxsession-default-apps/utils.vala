/* 
    Copyright 2013 Julien Lavergne <gilir@ubuntu.com>

    GTK3‑clean rewritten by iloveengines1234

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

namespace LDefaultApps
{
    /* Simple signal container */
    public class LDefaultAppsSignals : Object
    {
        public signal void update_ui ();
    }

    /* Load or create a KeyFile safely */
    public KeyFile load_key_conf (string config_path_directory, string conf_name)
    {
        var kf = new KeyFile ();

        /* Ensure directory exists */
        var config_dir = File.new_for_path (config_path_directory);
        if (!config_dir.query_exists ())
        {
            try
            {
                config_dir.make_directory_with_parents ();
            }
            catch (Error e)
            {
                stderr.printf ("Could not create config directory: %s\n", e.message);
            }
        }

        /* Ensure file exists */
        string config_path = Path.build_filename (config_path_directory, conf_name);
        var config_file = File.new_for_path (config_path);

        if (!config_file.query_exists ())
        {
            try
            {
                /* PRIVATE = 0600 permissions */
                config_file.create (FileCreateFlags.PRIVATE);
            }
            catch (Error e)
            {
                stderr.printf ("Could not create settings file: %s\n", e.message);
            }
        }

        /* Load KeyFile */
        try
        {
            kf.load_from_file (config_path, KeyFileFlags.NONE);
        }
        catch (Error e)
        {
            /* KeyFileError and FileError both derive from GLib.Error now */
            warning ("Error loading keyfile: %s", e.message);
        }

        return kf;
    }
}
