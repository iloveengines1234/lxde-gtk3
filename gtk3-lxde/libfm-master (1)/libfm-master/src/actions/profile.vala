//      profile.vala
//      
//      Copyright 2011 Hong Jen Yee (PCMan) <pcman.tw@pcman.tw@gmail.com>
//      
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either version 2 of the License, or
//      (at your option) any later version.
//      
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//      
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.
//      
//      

namespace Fm {

public enum FileActionExecMode {
	NORMAL,
	TERMINAL,
	EMBEDDED,
	DISPLAY_OUTPUT
}

[Compact]
public class FileActionProfile {

	public string id;
	public string? name;
	public string exec;
	public string? path;
	public FileActionExecMode exec_mode;
	public bool startup_notify;
	public string? startup_wm_class;
	public string? exec_as;
	public FileActionCondition condition;

	public FileActionProfile(KeyFile kf, string profile_name) {
		id = profile_name;
		string group_name = @"X-Action-Profile $profile_name";
		name = Utils.key_file_get_string(kf, group_name, "Name");
		exec = Utils.key_file_get_string(kf, group_name, "Exec");

		path = Utils.key_file_get_string(kf, group_name, "Path");
		var s = Utils.key_file_get_string(kf, group_name, "ExecutionMode");
		if(s == "Normal")
			exec_mode = FileActionExecMode.NORMAL;
		else if(s == "Terminal")
			exec_mode = FileActionExecMode.TERMINAL;
		else if(s == "Embedded")
			exec_mode = FileActionExecMode.EMBEDDED;
		else if(s == "DisplayOutput")
			exec_mode = FileActionExecMode.DISPLAY_OUTPUT;
		else
			exec_mode = FileActionExecMode.NORMAL;

		startup_notify = Utils.key_file_get_bool(kf, group_name, "StartupNotify");
		startup_wm_class = Utils.key_file_get_string(kf, group_name, "StartupWMClass");
		exec_as = Utils.key_file_get_string(kf, group_name, "ExecuteAs");

		condition = new FileActionCondition(kf, group_name);
	}

	private bool launch_once(AppLaunchContext ctx, FileInfo? first_file, List<FileInfo> files, out string? output) {
		output = null;
		if(exec == null)
			return false;

		var exec_cmd = FileActionParameters.expand(exec, files, false, first_file);
		bool ret = false;

		if(exec_mode == FileActionExecMode.DISPLAY_OUTPUT) {
			try {
				int exit_status;
				string stdout_buf;
				// Synchronously grab console buffers cleanly
				ret = Process.spawn_command_line_sync(exec_cmd, out stdout_buf, null, out exit_status);
				if(ret) {
					ret = (exit_status == 0);
					output = stdout_buf;
				}
			}
			catch(SpawnError err) {
				ret = false;
			}
		}
		else {
			try {
				// Fixed: Handle desktop environment context forwarding manually 
				// to avoid GAppInfo double-expansion conflicts while maintaining GTK3 window telemetry.
				string[] argv;
				if (Shell.parse_argv(exec_cmd, out argv)) {
					SpawnFlags spawn_flags = SpawnFlags.SEARCH_PATH;
					
					// Apply desktop terminal execution parameters if specified
					if (exec_mode == FileActionExecMode.TERMINAL || exec_mode == FileActionExecMode.EMBEDDED) {
						// Fallback sequence tracing common to lightweight X environments like LXDE
						string[] term_argv = {"x-terminal-emulator", "-e"};
						if (Environment.find_program_in_path("lxterminal") != null) {
							term_argv[0] = "lxterminal";
						}
						
						string[] complete_cmd = new string[term_argv.length + argv.length];
						long idx = 0;
						foreach(string t_arg in term_argv) complete_cmd[idx++] = t_arg;
						foreach(string a_arg in argv) complete_cmd[idx++] = a_arg;
						argv = complete_cmd;
					}

					Pid child_pid;
					ret = Process.spawn_async(
						path != null && path != "" ? path : null,
						argv,
						null,
						spawn_flags,
						ctx.child_setup,
						out child_pid
					);
					
					if (ret && startup_notify) {
						// Tell the display context a process was initiated
						ctx.launch_failed(id); 
					}
				}
			}
			catch(Error err) {
				ret = false;
			}
		}
		return ret;
	}

	public bool launch(AppLaunchContext ctx, List<FileInfo> files, out string? output) {
		output = null;
		if (files == null || files.length() == 0)
			return false;

		bool plural_form = FileActionParameters.is_plural(exec);
		bool ret;

		if(plural_form) { 
			ret = launch_once(ctx, files.first().data, files, out output);
		}
		else { 
			var all_output = new StringBuilder();
			ret = true;

			foreach(unowned FileInfo fi in files) {
				string? one_output;
				// Create a temporary singular list containing just this file to satisfy expansion requirements
				var singular_list = new List<FileInfo>();
				singular_list.append(fi);

				bool res = launch_once(ctx, fi, singular_list, out one_output);
				if (!res) ret = false;

				if(one_output != null && one_output != "") {
					// Fixed: Aggregate single outputs cleanly without causing buffer bloat
					all_output.append(one_output);
					if (!one_output.has_suffix("\n")) {
						all_output.append_c('\n');
					}
				}
			}
			
			if (all_output.len > 0) {
				output = all_output.str;
			}
		}
		return ret;
	}

	public bool match(List<FileInfo> files) {
		return condition.match(files);
	}
}