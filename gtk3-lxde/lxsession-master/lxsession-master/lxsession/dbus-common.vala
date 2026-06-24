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

namespace Lxsession
{
    // Define the standardized system execution target interface
    public interface LogindInterface : Object {
        public abstract void power_off() throws IOError;
        public abstract void reboot() throws IOError;
        public abstract async string can_power_off() throws IOError;
        public abstract async string can_reboot() throws IOError;
    }

    [DBus (name = "org.freedesktop.ConsoleKit.Manager")]
    public interface ConsoleKitObject : Object {
        public abstract void restart() throws IOError;
        public abstract void stop() throws IOError;
        public abstract async bool can_restart() throws IOError;
        public abstract async bool can_stop() throws IOError;
    }

    public class SessionObject : Object {
        private LogindInterface? logind_proxy = null;
        private ConsoleKitObject? consolekit_proxy = null;
        private bool use_logind = false;

        public SessionObject() {
            // First attempt to bind to systemd-logind (modern standard)
            try {
                logind_proxy = GLib.Bus.get_proxy_sync<LogindInterface>(
                    BusType.SYSTEM,
                    "org.freedesktop.login1",
                    "/org/freedesktop/login1",
                    DBusProxyFlags.NONE,
                    null
                );
                use_logind = true;
                message("%s", "Successfully initialized systemd-logind back-end proxy platform connection");
            } catch (IOError e) {
                debug("Systemd-logind not available (%s). Falling back to ConsoleKit...", e.message);
                use_logind = false;
            }

            // Fall back to legacy ConsoleKit if logind is absent
            if (!use_logind) {
                try {
                    consolekit_proxy = GLib.Bus.get_proxy_sync<ConsoleKitObject>(
                        BusType.SYSTEM,
                        "org.freedesktop.ConsoleKit",
                        "/org/freedesktop/ConsoleKit/Manager",
                        DBusProxyFlags.NONE, // Fixed capitalization typo
                        null
                    );
                    message("%s", "Successfully initialized fallback ConsoleKit management target interface wrapper infrastructure");
                } catch (IOError ex) {
                    critical("Fatal: Neither systemd-logind nor ConsoleKit session interfaces could be resolved: %s", ex.message);
                }
            }
        }

        public async bool lxsession_can_shutdown() {
            if (use_logind && logind_proxy != null) {
                try {
                    // Logind returns strings like "yes", "no", "challenge"
                    string response = yield logind_proxy.can_power_off();
                    return (response == "yes" || response == "challenge");
                } catch (IOError err) {
                    warning("Logind status lookup verification failed: %s", err.message);
                    return false;
                }
            } else if (consolekit_proxy != null) {
                try {
                    return yield consolekit_proxy.can_stop();
                } catch (IOError err) {
                    warning("ConsoleKit status lookup verification failed: %s", err.message);
                    return false;
                }
            }
            return false;
        }

        public void lxsession_shutdown() {
            try {
                if (use_logind && logind_proxy != null) {
                    logind_proxy.power_off();
                } else if (consolekit_proxy != null) {
                    consolekit_proxy.stop();
                }
            } catch (IOError err) {
                warning("Execution of System Shutdown Request failed: %s", err.message);
            }
        }

        public void lxsession_restart() {
            try {
                if (use_logind && logind_proxy != null) {
                    logind_proxy.reboot();
                } else if (consolekit_proxy != null) {
                    consolekit_proxy.restart();
                }
            } catch (IOError err) {
                warning("Execution of System Restart Request failed: %s", err.message);
            }
        }
    }

    // Fixed misspelled callback signature names and populated structural dependency wrappers
    public void on_bus_acquired(DBusConnection conn, GlobalSettingsProvider settings, GlobalSignalProxy signals) {
        try {
            // Instantiated with parameters tracking actual infrastructure boundaries
            var lxde_server = new LxdeSessionServer(settings, signals);
            conn.register_object("/org/lxde/SessionManager", lxde_server);
        } catch (IOError e) {
            stderr.printf("Could not register LXDE service interface registration mappings: %s\n", e.message);
        }
    }

    public void on_gnome_bus_acquired(DBusConnection conn) {
        try {
            // Assumes a corresponding GnomeSessionServer exists within standard operational scope boundaries
            // conn.register_object("/org/gnome/SessionManager", new GnomeSessionServer());
        } catch (IOError e) {
            stderr.printf("Could not register GNOME session backend alignment layer structures: %s\n", e.message);
        }
    }
}