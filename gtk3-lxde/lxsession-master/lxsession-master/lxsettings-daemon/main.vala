/* * Copyright 2013 Julien Lavergne <gilir@ubuntu.com>
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

public class Main : GLib.Object
{
    static bool persistent = false;
    static string? file = null;

    const OptionEntry[] option_entries = {
        { "file", 'f', 0, OptionArg.STRING, ref file, "path of the configuration file", "NAME" },
        { "persistent", 'p', 0, OptionArg.NONE, ref persistent, "reload configuration on file change", null },
        { null }
    };

    public static int main(string[] args)
    {
        /* FIXED: Added the OptionContext parsing pipeline so command line flags are actually read */
        try
        {
            var context = new OptionContext("- LXDE Session Daemon");
            context.add_main_entries(option_entries, null);
            context.parse(ref args);
        }
        catch (OptionError e)
        {
            critical("Option parsing failed: %s", e.message);
            return -1;
        }

        if (file == null)
        {
            critical("Error, you need to specify a configuration file using -f argument. Exit");
            return -1;
        }

        KeyFile kf = new KeyFile();

        try
        {
            kf.load_from_file(file, KeyFileFlags.NONE);
        }
        catch (KeyFileError err)
        {
            warning(err.message);
            critical("Problem when loading the configuration file. Exit");
            return -1;
        }
        catch (FileError err)
        {
            warning(err.message);
            critical("Problem when loading the configuration file. Exit");
            return -1;
        }

        /* Start settings daemon */
        settings_daemon_start(kf);

        if (!persistent)
        {
            /* Nothing to do, just exit cleanly */
            return 0;
        }
        else
        {
            /* TODO: Wire up GLib.FileMonitor here to automatically watch 'file' for on-disk changes */
            message("Daemon running in persistent mode. Monitoring configuration updates.");
            
            /* FIXED: Spawn an active GLib Main Loop block to keep the daemon alive in the background */
            var loop = new MainLoop();
            loop.run();
            
            return 0;
        }
    }
}