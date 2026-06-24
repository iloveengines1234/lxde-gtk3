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

using Gtk;

namespace Lxsession
{
    public class Main : GLib.Object
    {
        static string? message = null;
        static string? type = null;

        const OptionEntry[] option_entries = {
            { "message", 'm', 0, OptionArg.STRING, ref message, "specify a string to be displayed as a message", "NAME" },
            { "type", 't', 0, OptionArg.STRING, ref type, "specify the type of the message (w = warning, i = info)", "NAME" },
            { null }
        };

        public static int main(string[] args)
        {
            try
            {
                var options_args = new OptionContext("- Lxsession message utility");
                options_args.set_help_enabled(true);
                options_args.add_main_entries(option_entries, null);
                options_args.parse(ref args);
            }
            catch (OptionError e)
            {
                critical("Option parsing failed: %s\n", e.message);
                return -1;
            }

            // Ensure a message string was supplied before spinning up a window context
            if (message == null || message == "")
            {
                critical("Error: You must specify a message string using the -m argument.\n");
                return -1;
            }

            Gtk.init(ref args);

            // FIXED: Dynamically map the message dialog style based on the type argument passed 
            Gtk.MessageType msg_type = Gtk.MessageType.INFO;
            string dialog_title = "Lxsession Info";

            if (type != null && type.down() == "w")
            {
                msg_type = Gtk.MessageType.WARNING;
                dialog_title = "Lxsession Warning";
            }

            /* FIXED: Instantiated the top-level MessageDialog directly. 
             * This completely removes the redundant, empty parent outer window artifact. */
            Gtk.MessageDialog msg = new Gtk.MessageDialog(
                null, 
                Gtk.DialogFlags.MODAL, 
                msg_type, 
                Gtk.ButtonsType.OK_CANCEL, 
                "%s", message
            );
            
            msg.title = dialog_title;
            msg.window_position = Gtk.WindowPosition.CENTER;

            msg.response.connect((response_id) =>
            {
                switch (response_id) {
                    case Gtk.ResponseType.OK:
                        stdout.printf("Ok\n");
                        break;
                    case Gtk.ResponseType.CANCEL:
                        stdout.printf("Cancel\n");
                        break;
                    default:
                        stdout.printf("Delete\n");
                        break;
                }
                // FIXED: Explicitly flush stdout buffers so calling parent wrappers read choices instantly
                stdout.flush();
                
                msg.destroy();
                Gtk.main_quit();
            });

            msg.show_all();

            /* start main loop */
            Gtk.main();

            return 0;
        }
    }
}