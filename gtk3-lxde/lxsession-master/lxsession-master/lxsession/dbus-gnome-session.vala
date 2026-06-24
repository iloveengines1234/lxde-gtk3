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

/* http://live.gnome.org/Vala/DBusServerSample#Using_GDBus */

namespace Lxsession
{
    [DBus(name = "org.gnome.SessionManager")]
    public class GnomeSessionServer : GLib.Object {
        
        private string not_implemented = "Error, lxsession doesn't implement this API";
        
        // Persistent control worker thread reference instance to manage unified window states securely
        private ControlObject control_backend;
        
        // Fix 2: Keep track of active application cookies using a stateful hash map table matching to window IDs
        private GLib.HashTable<uint, uint64?> inhibitor_map;

        public int something { get; set; }

        // Fix 4: Dropped invalid 'out' modifiers from native D-Bus signal event definitions
        public signal void ClientAdded(string path);
        public signal void ClientRemoved(string path);
        public signal void InhibitorAdded(string path);
        public signal void InhibitorRemoved(string path);
        public signal void SessionRunning();
        public signal void SessionOver();

        public GnomeSessionServer() {
            this.control_backend = new ControlObject();
            this.inhibitor_map = new GLib.HashTable<uint, uint64?>(GLib.int_hash, GLib.int_equal);
        }

        public void Setenv(string value) {
            message("%s: Setenv(%s)", this.not_implemented, value);
        }

        public void InitializationError(string mess, bool fatal) {
            message("%s: InitializationError(%s, %s)", this.not_implemented, mess, fatal.to_string());
        }

        // Fix 1: Removed invalid async modifier from unimplemented methods to allow clean processing
        public void RegisterClient(string app_id, string client_startup_id, out string client_id) {
            message("%s: RegisterClient(%s)", this.not_implemented, app_id);
            client_id = "/org/gnome/SessionManager/Client/Invalid";
        }

        public void UnregisterClient(string client_id) {
            message("%s: UnregisterClient(%s)", this.not_implemented, client_id);
        }

        // Fix 1 & 3: Synchronous signature execution with explicit 64-bit Window ID parsing
        public uint Inhibit(string app_id, uint64 toplevel_xid, string reason, uint flags) {
            message("Session Manager Inhibit requested by application: %s (Reason: %s, Flags: %u)", app_id, reason, flags);
            
            // Flag 8 maps directly to inhibiting the session being marked as idle (Screensaver suspension)
            if ((flags & 8) != 0) {
                uint cookie;
                // Generate a unique non-zero random token identifier securely
                do {
                    cookie = Random.next_int();
                } while (cookie == 0 || this.inhibitor_map.contains(cookie));

                // Save tracking records before processing underlying backend pipeline
                this.inhibitor_map.insert(cookie, toplevel_xid);
                this.control_backend.set_status_busy(toplevel_xid);
                
                return cookie;
            }

            // Return fallback 0 code if flags don't require screensaver overrides
            return 0;
        }

        // Fix 1 & 2: Safe, synchronous cookie evaluation matching target descriptors
        public void Uninhibit(uint inhibit_cookie) {
            if (inhibit_cookie == 0) return;

            if (this.inhibitor_map.contains(inhibit_cookie)) {
                uint64 toplevel_xid = this.inhibitor_map.lookup(inhibit_cookie);
                
                // Invoke specific termination via your production-ready tracking backend safely
                this.control_backend.exit_status_busy(toplevel_xid);
                this.inhibitor_map.remove(inhibit_cookie);
                
                message("Revoked active D-Bus session suspension lock via Cookie token ID: %u", inhibit_cookie);
            } else {
                warning("Application attempted to clear an un-registered or missing inhibitor cookie handle: %u", inhibit_cookie);
            }
        }

        public void Shutdown() {
            var session = new SessionObject();
            session.lxsession_shutdown();
        }

        // Fix 1: Properly formulated async method execution leveraging the yield command sequence
        public async bool CanShutdown() {
            var session = new SessionObject();
            try {
                bool is_available = yield session.lxsession_can_shutdown();
                return is_available;
            } catch (GLib.Error err) {
                warning("Asynchronous shutdown state lookup execution failed: %s", err.message);
                return false;
            }
        }

        public void Logout(uint mode) {
            stdout.printf("%s: Logout with mode %u\n", this.not_implemented, mode);
        }

        public bool IsSessionRunning() {
            return true; 
        }
    }
}