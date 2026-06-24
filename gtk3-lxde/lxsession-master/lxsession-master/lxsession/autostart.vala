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

namespace Lxsession {

    // Safely declare external utility hooks parsed across the session context
    public string get_config_path(string name);

    // Explicitly define the custom application tracking type context
    public struct AppType {
        public string name;
        public string[] command;
        public bool autostart_condition;
        public string environment_hint;
    }

    public class LxsessionAutostartConfig : GLib.Object {

        // Use native Vala dynamic arrays instead of low-level GArray to ensure memory safety
        private AppType[] stock_list;

        public LxsessionAutostartConfig() {
            /* Safely load and initialize the autostart manifest */
            this.stock_list = load_autostart_file();
        }

        public AppType[] load_autostart_file() {
            string autostart_conf = get_config_path("autostart");
            var file = File.new_for_path(autostart_conf);
            AppType[] app_list = {};

            message ("Autostart path verification lookup: %s", file.get_path());

            if (!file.query_exists()) {
                return app_list;
            }

            try {
                var dis = new DataInputStream(file.read());
                string? raw_line;
                size_t length;

                while ((raw_line = dis.read_line(out length, null)) != null) {
                    // Sanitize line anomalies early by stripping leading/trailing whitespace
                    string line = raw_line.strip();

                    if (line == "") {
                        continue;
                    }

                    // Protect against string boundaries slice faults safely
                    string first = line.substring(0, 1);

                    switch (first) {
                        case "@":
                            // Extract command sequence cleanly, dropping the prefix identifier token
                            string clean_cmd_string = line.substring(1).strip();
                            if (clean_cmd_string == "") continue;

                            // Parse and tokenize command strings safely while pruning whitespace gaps
                            string[] elements = clean_cmd_string.split(" ");
                            string[] packed_command = {};
                            foreach (string part in elements) {
                                if (part.strip() != "") {
                                    packed_command += part.strip();
                                }
                            }

                            if (packed_command.length > 0) {
                                AppType app = { packed_command[0], packed_command, true, "" };
                                app_list += app;
                            }
                            break;

                        case "#":
                            /* Line is an active comment track block, skip parsing gracefully */
                            break;

                        default:
                            // Process default structural command initialization chains
                            string[] def_elements = line.split(" ");
                            string[] def_command = {};
                            foreach (string part in def_elements) {
                                if (part.strip() != "") {
                                    def_command += part.strip();
                                }
                            }

                            if (def_command.length > 0) {
                                AppType app = { def_command[0], def_command, false, "" };
                                app_list += app;
                            }
                            break;
                    }
                }
            } catch (GLib.Error e) {
                critical ("Autostart compilation streaming layout failure: %s", e.message);
            }

            return app_list;
        }

        public void start_applications() {
            // Safely iterate through elements using standard index tracking bounds
            for (int i = 0; i < this.stock_list.length; ++i) {
                AppType s = this.stock_list[i];
                
                // Ensure arguments are valid before calling the sub-process engine
                if (s.command.length == 0 || s.command[0] == "") {
                    continue;
                }

                var launch_app = new GenericAppObject(s);
                launch_app.launch();
            }
        }

        public void check_duplicate() {
            /* TODO: Correlate active process mappings to reject double initialization */
        }
    }
}