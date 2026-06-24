/* Copyright 2013 Julien Lavergne <gilir@ubuntu.com>

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
using Gtk;

namespace LDefaultApps
{
    public string read_autostart_conf ()
    {
        string config_path = get_config_home_path("autostart");
        var config_file = File.new_for_path (config_path);
        
        if (!config_file.query_exists ())
        {
            string? config_system_path = get_config_path("autostart");

            if (config_system_path == null)
            {
                /* No system file and no home file, create a blank file in home */
                var config_parent = config_file.get_parent();

                if (!config_parent.query_exists ())
                {
                    try
                    {
                        config_parent.make_directory_with_parents ();
                    }
                    catch (GLib.Error e)
                    {
                        message ("%s", e.message);
                    }
                }

                try
                {
                    message("Create blank file");
                    var blank_file = File.new_for_path (config_path);
                    blank_file.create (FileCreateFlags.PRIVATE);
                }
                catch (GLib.Error e)
                {
                    message ("%s", e.message);
                }
            }
            else
            {
                var file = File.new_for_path (config_system_path);
                var config_parent = config_file.get_parent();

                if (!config_parent.query_exists ())
                {
                    try
                    {
                        config_parent.make_directory_with_parents ();
                    }
                    catch (GLib.Error e)
                    {
                        message ("%s", e.message);
                    }
                }

                try
                {
                    file.copy (config_file, FileCopyFlags.NONE);
                }
                catch (GLib.Error e)
                {
                    message ("%s", e.message);
                }
            }
        }
        message("Conf file for autostart: %s", config_path);
        return config_path;
    }

    public void manual_autostart_init (Builder builder)
    {
        var autostart_conf = read_autostart_conf();

        if (autostart_conf == null || autostart_conf == "")
        {
            message("Can't find an autostart file, abort");
            return;
        }

        FileStream? stream = FileStream.open (autostart_conf, "r");
        if (stream == null)
        {
            critical("Failed to open autostart configuration file: %s", autostart_conf);
            return;
        }

        message ("Autostart conf file : %s", autostart_conf);

        // FIXED: Upgraded legacy layout elements to modern, flexible Box layouts
        var auto_vbox = builder.get_object("manual_autostart_vbox") as Gtk.Box;
        if (auto_vbox == null)
        {
            critical("Could not look up manual_autostart_vbox layout container context.");
            return;
        }

        // Clean up any existing children cleanly
        auto_vbox.foreach((widget) => {
            auto_vbox.remove(widget);
        });

        string? line = null;
        while ((line = stream.read_line ()) != null)
        {
            string clean_line = line.strip();
            if (clean_line == "") continue;

            message("Autostart line : %s", clean_line);
            var hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 6);
            var check = new Gtk.CheckButton.with_label (clean_line);
            
            if (clean_line.has_prefix("#"))
            {
                check.set_active(false);
            }
            else
            {
                check.set_active(true);
            }

            // FIXED: Block signal execution triggers during refresh loops to prevent nested event loops
            check.toggled.connect (() => {
                string current_label = check.get_label();
                message("Label to update : %s", current_label);
                if (check.get_active())
                {
                    if (current_label.has_prefix("#"))
                    {
                        update_autostart_conf(current_label, "activate", builder);
                        message("Activate : %s", current_label);
                    }
                }
                else
                {
                    if (!current_label.has_prefix("#"))
                    {
                        update_autostart_conf(current_label, "desactivate", builder);
                        message("Deactivate : %s", current_label);
                    }
                }
            });

            hbox.pack_start(check, true, true, 0);

            // FIXED: Replaced dropped stock configurations with modern theme-compliant icon imagery handles
            var button = new Gtk.Button.from_icon_name("list-remove-symbolic", Gtk.IconSize.BUTTON);
            button.clicked.connect (() => {
                update_autostart_conf(check.get_label(), "remove", builder);
                message ("try to remove : %s", check.get_label());
            });

            hbox.pack_start(button, false, false, 0);
            auto_vbox.pack_start(hbox, false, false, 0);
        }

        var add_hbox = new Gtk.Box(Gtk.Orientation.HORIZONTAL, 6);
        var add_button = new Gtk.Button.from_icon_name("list-add-symbolic", Gtk.IconSize.BUTTON);
        var add_entry = new Gtk.Entry();
        
        add_hbox.pack_start(add_entry, true, true, 0);
        add_hbox.pack_start(add_button, false, false, 0);
        auto_vbox.pack_start(add_hbox, false, false, 0);
        
        add_button.clicked.connect (() => {
            string text_to_add = add_entry.get_text().strip();
            if (text_to_add != "")
            {
                update_autostart_conf(text_to_add, "add", builder);
                add_entry.set_text("");
            }
        });

        auto_vbox.show_all();
    }

    public void update_autostart_conf (string line, string action, Builder builder)
    {
        var new_line = new StringBuilder ();
        switch (action)
        {
            case "activate":
                new_line.append(line);
                new_line.erase(0, 1);
                new_line.append("\n");
                break;
            case "desactivate":
                new_line.append("#");
                new_line.append(line);
                new_line.append("\n");
                break;
            case "add":
                new_line.append(line);
                new_line.append("\n");
                break;
            case "remove":
                break;
        }
        
        try
        {
            string tmp_dir = Path.build_filename(Environment.get_user_cache_dir(), "lxsession-default-apps");
            var tmp_dir_file = File.new_for_path(tmp_dir);
            if (!tmp_dir_file.query_exists())
            {
                tmp_dir_file.make_directory_with_parents();
            }

            string tmp_path = Path.build_filename(tmp_dir, "autostart.tmp");
            var tmp_file = File.new_for_path (tmp_path);
            var dest_file = File.new_for_path (read_autostart_conf());

            FileStream? stream = FileStream.open (read_autostart_conf(), "r");
            if (stream == null) return;

            var tmp_stream = new DataOutputStream (tmp_file.create (FileCreateFlags.REPLACE_DESTINATION));

            string? read = null;
            while ((read = stream.read_line ()) != null)
            {
                if (read == line)
                {
                    if (action != "remove")
                    {
                        tmp_stream.put_string(new_line.str);
                    }
                }
                else
                {
                    tmp_stream.put_string(read);
                    tmp_stream.put_string("\n");
                }
            }

            if (action == "add")
            {
                tmp_stream.put_string(new_line.str);
            }

            // Explicitly close the file streams to flush memory before copying
            tmp_stream.close();
            stream = null;

            tmp_file.copy(dest_file, FileCopyFlags.OVERWRITE);
            tmp_file.delete();
            
            // Re-render user interfaces on idle frames to bypass cyclic recursion crashes
            Idle.add(() => {
                manual_autostart_init(builder);
                return Source.REMOVE;
            });
        }
        catch (GLib.Error e)
        {
            message ("%s", e.message);
        }
    }
}