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
    // Safely pull asynchronous process execution pipelines provided across the project
    public void lxsession_spawn_command_line_async(string command);

    public class ControlObject : GLib.Object
    {
        // Fix 2: Keep track of active suspension cookie handles dynamically to protect application states
        private uint64[] active_inhibitors = {};

        /**
         * Status : Busy doing something, disable idle behavior of application
         */
        public void set_status_busy(uint64 toplevel_xid)
        {
            this.inhib_screensaver(toplevel_xid);
        }

        public void exit_status_busy(uint64 toplevel_xid)
        {
            this.uninhibit_screensaver(toplevel_xid);
        }

        public void inhib_screensaver(uint64 toplevel_xid)
        {
            // Fix 3: Handle 0 value exceptions securely
            if (toplevel_xid == 0)
            {
                warning("Attempted to register screensaver inhibitor with an uninitialized or empty XID.");
                return;
            }

            // Check if this window identity is already registered to avoid duplication
            if (toplevel_xid in this.active_inhibitors)
            {
                debug("Window identifier %s is already tracking active suspension.", toplevel_xid.to_string());
                return;
            }

            // Formulate standard compliant XDG execution command flags safely
            string create_command = "xdg-screensaver suspend %s".printf(toplevel_xid.to_string());
            lxsession_spawn_command_line_async(create_command);
            
            this.active_inhibitors += toplevel_xid;
            message("Successfully registered screensaver inhibitor handle for Window XID: %s", toplevel_xid.to_string());
        }

        public void uninhibit_screensaver(uint64 toplevel_xid)
        {
            if (toplevel_xid == 0) return;

            // Fix 1 & 2: Search and drop the matching targeted window descriptor to prevent global lock corruption
            int index = -1;
            for (int i = 0; i < this.active_inhibitors.length; i++)
            {
                if (this.active_inhibitors[i] == toplevel_xid)
                {
                    index = i;
                    break;
                }
            }

            if (index != -1)
            {
                // Fix 1: Execute standard compliant resume routines passing explicit identifier requirements
                string resume_command = "xdg-screensaver resume %s".printf(toplevel_xid.to_string());
                lxsession_spawn_command_line_async(resume_command);

                // Re-slice dynamic array constraints seamlessly in native Vala syntax
                uint64[] updated_list = {};
                for (int i = 0; i < this.active_inhibitors.length; i++)
                {
                    if (i != index)
                    {
                        updated_list += this.active_inhibitors[i];
                    }
                }
                this.active_inhibitors = updated_list;
                message("Successfully revoked screensaver inhibitor handle for Window XID: %s", toplevel_xid.to_string());
            }
            else
            {
                warning("Requested suspension release for an unregistered or untracked Window handle: %s", toplevel_xid.to_string());
            }
        }
        
        /**
         * Global emergency backup routine to clean up un-dropped tokens during logout operations
         */
        public void clear_all_inhibitors()
        {
            if (this.active_inhibitors.length == 0) return;
            
            debug("Purging all active session inhibitor locks before termination sequence.");
            foreach (uint64 xid in this.active_inhibitors)
            {
                string resume_command = "xdg-screensaver resume %s".printf(xid.to_string());
                lxsession_spawn_command_line_async(resume_command);
            }
            this.active_inhibitors = {};
        }
    }
}