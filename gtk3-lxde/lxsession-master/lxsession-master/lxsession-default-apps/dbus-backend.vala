/* Copyright 2012 Julien Lavergne <gilir@ubuntu.com>
    Updated for modern GLib/GIO (GTK3 ecosystem)

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
    [DBus(name = "org.lxde.SessionManager")]
    public interface DbusLxsession : GLib.Object
    {
        public abstract string SessionGet (string key1, string? key2) throws GLib.Error;
        public abstract void SessionSet (string key1, string? key2, string command_to_set) throws GLib.Error;
        public abstract void SessionLaunch (string name, string option) throws GLib.Error;
        public abstract string[] SessionSupport () throws GLib.Error;
        public abstract string[] SessionSupportDetail (string key1) throws GLib.Error;

        public abstract void KeymapActivate () throws GLib.Error;
        public abstract void KeymapSet (string key1, string? key2, string command_to_set) throws GLib.Error;
        public abstract string KeymapGet (string key1, string? key2) throws GLib.Error;

        public abstract void StateSet (string key1, string? key2, string command_to_set) throws GLib.Error;
        public abstract string StateGet (string key1, string? key2) throws GLib.Error;

        public abstract void DbusSet (string key1, string? key2, string command_to_set) throws GLib.Error;
        public abstract string DbusGet (string key1, string? key2) throws GLib.Error;

        public abstract void EnvironmentSet (string key1, string? key2, string command_to_set) throws GLib.Error;
        public abstract string EnvironmentGet (string key1, string? key2) throws GLib.Error;
    }

    public class DbusBackend : GLib.Object
    {
        private DbusLxsession? dbus_lxsession = null;
        private string dbus_type;

        public DbusBackend (string? type)
        {
            this.dbus_type = type ?? "session";

            try
            {
                // Modern GIO D-Bus proxy resolution
                this.dbus_lxsession = Bus.get_proxy_sync (
                    BusType.SESSION,
                    "org.lxde.SessionManager",
                    "/org/lxde/SessionManager"
                );
            }
            catch (GLib.Error err)
            {
                warning ("Failed to connect to SessionManager D-Bus: %s", err.message);
            }
        }

        public string Get (string key1, string? key2)
        {
            if (this.dbus_type == "session")
            {
                return this.SessionGet (key1, key2);
            }
            return "";
        }

        public void Set (string key1, string? key2, string command_to_set)
        {
            if (this.dbus_type == "session")
            {
                this.SessionSet (key1, key2, command_to_set);
            }
        }

        public void Launch (string name, string option)
        {
            if (this.dbus_type == "session")
            {
                this.SessionLaunch (name, option);
            }
        }

        /* ─── SESSION METHODS ────────────────────────────────────────────────── */

        public string? SessionGet (string key1, string? key2)
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.SessionGet (key1, key2 ?? "");
            }
            catch (GLib.Error err)
            {
                warning ("SessionGet error: %s", err.message);
                return null;
            }
        }

        public void SessionLaunch (string name, string option)
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.SessionLaunch (name, option);
            }
            catch (GLib.Error err)
            {
                warning ("SessionLaunch error: %s", err.message);
            }
        }

        public void SessionSet (string key1, string? key2, string command_to_set)
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.SessionSet (key1, key2 ?? "", command_to_set);
            }
            catch (GLib.Error err)
            {
                warning ("SessionSet error: %s", err.message);
            }
        }

        public string[]? SessionSupport ()
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.SessionSupport ();
            }
            catch (GLib.Error err)
            {
                warning ("SessionSupport error: %s", err.message);
                return null;
            }
        }

        public string[]? SessionSupportDetail (string key1)
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.SessionSupportDetail (key1);
            }
            catch (GLib.Error err)
            {
                warning ("SessionSupportDetail error: %s", err.message);
                return null;
            }
        }

        /* ─── KEYMAP METHODS ─────────────────────────────────────────────────── */

        public void KeymapActivate ()
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.KeymapActivate ();
            }
            catch (GLib.Error err)
            {
                warning ("KeymapActivate error: %s", err.message);
            }
        }

        public void KeymapSet (string key1, string? key2, string command_to_set)
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.KeymapSet (key1, key2 ?? "", command_to_set);
            }
            catch (GLib.Error err)
            {
                warning ("KeymapSet error: %s", err.message);
            }
        }

        public string? KeymapGet (string key1, string? key2)
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.KeymapGet (key1, key2 ?? "");
            }
            catch (GLib.Error err)
            {
                warning ("KeymapGet error: %s", err.message);
                return null;
            }
        }

        /* ─── STATE METHODS ──────────────────────────────────────────────────── */

        public void StateSet (string key1, string? key2, string command_to_set)
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.StateSet (key1, key2 ?? "", command_to_set);
            }
            catch (GLib.Error err)
            {
                warning ("StateSet error: %s", err.message);
            }
        }

        public string? StateGet (string key1, string? key2)
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.StateGet (key1, key2 ?? "");
            }
            catch (GLib.Error err)
            {
                warning ("StateGet error: %s", err.message);
                return null;
            }
        }

        /* ─── DBUS METHODS ───────────────────────────────────────────────────── */

        public void DbusSet (string key1, string? key2, string command_to_set)
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.DbusSet (key1, key2 ?? "", command_to_set);
            }
            catch (GLib.Error err)
            {
                warning ("DbusSet error: %s", err.message);
            }
        }

        public string? DbusGet (string key1, string? key2)
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.DbusGet (key1, key2 ?? "");
            }
            catch (GLib.Error err)
            {
                warning ("DbusGet error: %s", err.message);
                return null;
            }
        }

        /* ─── ENVIRONMENT METHODS ────────────────────────────────────────────── */

        public void EnvironmentSet (string key1, string? key2, string command_to_set)
        {
            if (this.dbus_lxsession == null) return;
            try
            {
                this.dbus_lxsession.EnvironmentSet (key1, key2 ?? "", command_to_set);
            }
            catch (GLib.Error err)
            {
                warning ("EnvironmentSet error: %s", err.message);
            }
        }

        public string? EnvironmentGet (string key1, string? key2)
        {
            if (this.dbus_lxsession == null) return null;
            try
            {
                return this.dbus_lxsession.EnvironmentGet (key1, key2 ?? "");
            }
            catch (GLib.Error err)
            {
                warning ("EnvironmentGet error: %s", err.message);
                return null;
            }
        }
    }
}