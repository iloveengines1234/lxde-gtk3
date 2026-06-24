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

namespace Lxsession {

    // Fix 2: Eliminated duplicate AppType struct definition to allow clean integration with autostart.vala

    public KeyFile load_keyfile(string config_path) {
        KeyFile kf = new KeyFile();
        try {
            kf.load_from_file(config_path, KeyFileFlags.NONE);
        } catch (GLib.Error err) {
            // Combined exceptions under a standard GLib.Error interface block safely
            warning("Configuration parser failed to stream source path (%s): %s", config_path, err.message);
        }
        return kf;
    }

    public string get_config_home_path(string conf_file) {
        return Path.build_filename(
            Environment.get_user_config_dir(),
            "lxsession",
            session_global ?? "LXDE",
            conf_file
        );
    }

    public string? get_config_path(string conf_file) {
        string user_config_dir = get_config_home_path(conf_file);

        if (FileUtils.test(user_config_dir, FileTest.EXISTS)) {
            message("User profile configuration override found: %s", user_config_dir);
            return user_config_dir;
        }

        string[] system_config_dirs = Environment.get_system_config_dirs();
        
        // Fix 1: Validate the full file path inside the loop rather than just checking the directory
        foreach (string config in system_config_dirs) {
            string candidate_path = Path.build_filename(config, "lxsession", session_global ?? "LXDE", conf_file);
            message("Searching system location path: %s", candidate_path);
            
            if (FileUtils.test(candidate_path, FileTest.EXISTS)) {
                message("System configuration matched successfully: %s", candidate_path);
                return candidate_path;
            }
        }
        
        warning("Requested configuration target file could not be located anywhere on this system: %s", conf_file);
        return null;
    }

    public class LxSignals : GLib.Object {
        public signal void update_window_manager(string dbus_arg, string kf_categorie = "Session", string kf_key1 = "window_manager", string? kf_key2 = null);
        /* Xsettings Signalling hooks */
        public signal void reload_settings_daemon();
        public signal void generic_set_signal(string categorie, string key1, string? key2, string type, string dbus_arg);
    }

    public bool detect_laptop() {
        string? test_laptop_detect = Environment.find_program_in_path("laptop-detect");
        if (test_laptop_detect == null) {
            message("System diagnostic framework 'laptop-detect' not found on local path environmental scopes.");
            return false;
        }

        int wait_status;
        string standard_output;
        string standard_error;

        try {
            Process.spawn_command_line_sync("laptop-detect", out standard_output, out standard_error, out wait_status);
            
            // Fix 4: Process waitpid binary statuses through GLib evaluation macros securely
            if (Process.if_exited(wait_status)) {
                int real_exit_code = Process.exit_status(wait_status);
                message("Laptop validation application returned exit code sequence: %d", real_exit_code);
                return (real_exit_code == 0);
            }
        } catch (GLib.SpawnError err) {
            warning("Process subsystem layout error running laptop-detect diagnostics: %s", err.message);
        }
        return false;
    }

    public bool check_package_manager_running() {
        // Fix 3: Implemented an active POSIX file lock verification routine
        string[] target_locks = {
            "/var/lib/dpkg/lock",
            "/var/cache/apt/archives/lock",
            "/var/lib/apt/lists/lock",
            "/var/run/unattended-upgrades.lock"
        };

        foreach (string lock_path in target_locks) {
            if (!FileUtils.test(lock_path, FileTest.EXISTS)) {
                continue;
            }

            // Attempt to open the path in read-only mode to check its lock status safely
            int fd = Posix.open(lock_path, Posix.O_RDONLY);
            if (fd < 0) {
                // If open fails with Permission Denied (EACCES), the file is likely locked by an active process
                if (Posix.errno == Posix.EACCES || Posix.errno == Posix.EPERM) {
                    message("Active package manager lock found (Access Denied) at path location: %s", lock_path);
                    return true;
                }
                continue;
            }

            // Test if an active write or read lock is applied over the target descriptor using POSIX flock
            struct Posix.flock fl = {};
            fl.l_type = Posix.F_WRLCK;
            fl.l_whence = Posix.SEEK_SET;
            fl.l_start = 0;
            fl.l_len = 0;

            if (Posix.fcntl(fd, Posix.F_GETLK, ref fl) < 0) {
                Posix.close(fd);
                continue;
            }

            Posix.close(fd);

            // If F_GETLK returns a type other than F_UNLCK, another process holds the system lock
            if (fl.l_type != Posix.F_UNLCK) {
                message("Package transaction thread detected on file path resource descriptor lock: %s (PID: %d)", lock_path, (int)fl.l_pid);
                return true;
            }
        }

        return false;
    }
}