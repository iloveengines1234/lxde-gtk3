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
    /* Facility for launching applications by extending the env variable set by lxsession.
       Ensures shell-compliant token parsing and clean zombie-reaping patterns.
    */
    public void lxsession_spawn_command_line_async(string command_line)
    {
        // Safeguard against uninitialized or entirely blank inputs
        if (command_line == null || command_line.strip() == "")
        {
            warning("%s", "Execution blocked: Attempted to spawn a null or empty command line string target.");
            return;
        }

        try
        {
            string[] command;
            
            // Fix 1: Shell-parse ensures parameters inside single/double quotes are accurately structured
            if (!Shell.parse_argv(command_line, out command))
            {
                warning("Parsing Error: Could not tokenize command line matching shell constraints: %s", command_line);
                return;
            }

            // Double check array bounds to eliminate any possible downstream segmentation faults
            if (command.length == 0)
            {
                warning("%s", "Parsing anomaly: Parsed command token list evaluated to zero length arrays.");
                return;
            }

            string[] spawn_env = Environ.get();
            Pid child_pid;

            // Fix 2: Explicitly pair matching execution flags safely
            Process.spawn_async(
                null,
                command,
                spawn_env,
                SpawnFlags.SEARCH_PATH | SpawnFlags.DO_NOT_REAP_CHILD,
                null,
                out child_pid
            );

            // Fix 3: Standardize GLib ChildWatch callbacks to clean up operational system PID resources
            ChildWatch.add(child_pid, (pid, status) => {
                // Ensure correct main loop threading binding synchronization context
                Process.close_pid(pid);
                debug("Child process lifecycle tracking closed cleanly for PID: %d with Exit Status: %d", (int)pid, status);
            });

        }
        catch (ShellError se)
        {
            warning("Shell compilation constraint failure tracked: %s", se.message);
        }
        catch (SpawnError err)
        {
            // Protect format arguments comprehensively from external environment variables
            warning("Process execution runtime platform layer error: %s", err.message);
            warning("Failed to successfully spawn target context binary executor routing path");
        }
    }
}