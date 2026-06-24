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
    /* Core App Object - Made abstract to prevent accidental direct instantiation */
    public abstract class AppObject : GLib.Object
    {
        public Pid pid;

        public string name { get; set; }
        public string[] command { get; set; }
        public bool guard { get; set; default = false; }
        public string application_type { get; set; }
        public int crash_count { get; set; default = 0; }

        /* Number of times the application can crash before stopping reload cycles */
        public int stop_reload { get; set; default = 5; }

        public AppObject()
        {
            // Removed constructor init() call to avoid early overriding execution side-effects
        }

        /* Enforce that launch can be safely handled polymorphically down the inheritance chain */
        public virtual void launch()
        {
            generic_launch(null);
        }

        public void generic_launch(string? arg1)
        {
            if (this.name != null && this.name != "")
            {
                try
                {
                    string[] spawn_env = Environ.get();
                    Process.spawn_async(
                        arg1,
                        this.command,
                        spawn_env,
                        SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD,
                        null,
                        out this.pid
                    );
                    
                    // Correctly map instance context to the ChildWatch handler frame
                    ChildWatch.add(this.pid, (pid, status) => {
                        this.callback_pid(pid, status);
                    });

                    message("Launching %s", this.name);
                    for (int a = 0; a < this.command.length; a++)
                    {
                        GLib.stdout.printf("%s ", this.command[a]);
                    }
                    GLib.stdout.printf("\n");
                }
                catch (SpawnError err)
                {
                    warning("%s", err.message);
                    warning("Error when launching %s", this.name);
                }
            }
        }

        public virtual void read_config_settings() {}
        public virtual void read_settings() {}

        public void stop()
        {
            if ((int)this.pid != 0)
            {
                message("Stopping process with pid %d", (int)this.pid);
                Posix.kill((int)this.pid, 15);
            }
        }

        public void reload()
        {
            message("Reloading process");
            this.stop();
            this.launch();
        }

        public void init()
        {
            read_config_settings();
            read_settings();
        }

        protected virtual void callback_pid(Pid pid, int status)
        {
            message("%s exit with this type of exit: %i", this.name, status);
            Process.close_pid(pid);

            if (this.guard)
            {
                switch (status)
                {
                    case 0:
                    case 256:
                        message("Exit normal, don't reload");
                        break;
                    case 15:
                        message("Exit by the user, don't reload");
                        break;
                    default:
                        message("Exit not normal, try to reload");
                        this.crash_count++;
                        if (this.crash_count <= this.stop_reload)
                        {
                            this.launch();
                        }
                        else
                        {
                            message("Application crashed too much, stop reloading");
                        }
                        break;
                }
            }
        }
    }

    public class SimpleAppObject : AppObject
    {
        public SimpleAppObject()
        {
            base();
            this.name = "";
            this.command = new string[] { "" };
            this.guard = false;
            this.application_type = "";
        }
    }

    public class GenericAppObject : AppObject
    {
        public GenericAppObject(AppType app_type)
        {
            base();
            this.name = app_type.name;
            this.command = app_type.command;
            this.guard = app_type.guard;
            this.application_type = app_type.application_type;
        }
    }

    public class GenericSimpleApp : SimpleAppObject
    {
        public string settings_command { get; set; default = ""; }

        public GenericSimpleApp(string argument)
        {
            base();
            settings_command = argument;
            // Safe to call now that instance fields are fully defined
            init();
        }

        public override void read_settings()
        {
            if (settings_command != null && settings_command != "")
            {
                string[] create_command = settings_command.split_set(" ", 0);
                this.name = create_command[0];
                this.command = create_command;
            }
        }
    }

    public class WindowsManagerApp : SimpleAppObject
    {
        private string wm_command;
        private string mode;
        private string session;
        private string extras;

        public WindowsManagerApp()
        {
            base();
            init();
        }

        public override void read_settings()
        {
            if (global_settings.get_item_string("Session", "window_manager", null) != null)
            {
                mode = "simple";
                wm_command = global_settings.get_item_string("Session", "window_manager", null);
                session = "";
                extras = "";
            }
            else
            {
                mode = "advanced";
                wm_command = global_settings.get_item_string("Session", "windows_manager", "command");
                session = global_settings.get_item_string("Session", "windows_manager", "session");
                extras = global_settings.get_item_string("Session", "windows_manager", "extras");
            }

            string? session_command = "";
            if (wm_command == "wm_safe")
            {
                this.name = "wm_safe";
                this.command = new string[] { find_window_manager() };
            }
            else
            {
                if (mode == "simple")
                {
                    this.name = wm_command;
                    this.command = new string[] { wm_command };
                }
                else
                {
                    this.name = wm_command;
                    string create_command = "";
                    string? xdg_config_env = Environment.get_variable("XDG_CONFIG_HOME");
                    if (xdg_config_env == null) xdg_config_env = Environment.get_home_dir() + "/.config";

                    switch (wm_command)
                    {
                        case "openbox":
                            switch (session)
                            {
                                case "LXDE":
                                    session_command = " --config-file " + xdg_config_env + "/openbox/lxde-rc.xml";
                                    break;
                                case "Lubuntu":
                                    session_command = " --config-file " + xdg_config_env + "/openbox/lubuntu-rc.xml";
                                    break;
                                default:
                                    session_command = " ";
                                    break;
                            }
                            break;
                        case "openbox-custom":
                            session_command = " --config-file " + session;
                            break;
                        default:
                            session_command = "";
                            break;
                    }

                    if (extras == null || extras.strip() == "")
                    {
                        create_command = wm_command + session_command;
                    }
                    else
                    {
                        create_command = wm_command + session_command + " " + extras;
                    }

                    this.command = create_command.split_set(" ", 0);
                }
            }
            this.guard = true;
        }

        private string find_window_manager()
        {
            string[] wm_list = {
                "openbox-lxde", "openbox-lubuntu", "openbox", "compiz",
                "kwin", "mutter", "fluxbox", "metacity", "xfwin", "matchbox"
            };

            foreach (string wm in wm_list)
            {
                string test_wm = Environment.find_program_in_path(wm);
                if (test_wm != null)
                {
                    message("Finding %s", wm);
                    return wm;
                }
            }
            return "";
        }

        protected override void callback_pid(Pid pid, int status)
        {
            message("%s exit with this type of exit: %i\n", this.name, status);
            if (status == -1)
            {
                this.name = "wm_safe";
                this.command = new string[] { find_window_manager() };
                global_settings.set_generic_default("Session", "windows_manager", "command", "string", "wm_safe");
            }
            base.callback_pid(pid, status);
        }

        /* Fix: Properly use override to ensure structural polymorphism matches base types */
        public override void launch()
        {
            this.read_config_settings();
            this.read_settings();

            if (this.name != null)
            {
                try
                {
                    string[] spawn_env = Environ.get();
                    Process.spawn_async(
                        null,
                        this.command,
                        spawn_env,
                        SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD,
                        null,
                        out this.pid
                    );
                    
                    ChildWatch.add(this.pid, (pid, status) => {
                        this.callback_pid(pid, status);
                    });

                    for (int a = 0; a < this.command.length; a++)
                    {
                        GLib.stdout.printf("%s ", this.command[a]);
                    }
                    GLib.stdout.printf("\n");
                }
                catch (SpawnError err)
                {
                    warning("%s", err.message);
                }
            }
        }
    }
}