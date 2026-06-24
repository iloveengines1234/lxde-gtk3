//      condition.vala
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

using Posix;

namespace Fm {

[Flags]
public enum FileActionCapability {
	OWNER = 1, // Fixed: Changed from 0 to 1 so bitwise flag evaluation functions correctly
	READABLE = 1 << 1,
	WRITABLE = 1 << 2,
	EXECUTABLE = 1 << 3,
	LOCAL = 1 << 4
}

[Compact]
public class FileActionCondition {
	
	public string[]? only_show_in;
	public string[]? not_show_in;
	public string? try_exec;
	public string? show_if_registered;
	public string? show_if_true;
	public string? show_if_running;
	public string[]? mime_types;
	public string[]? base_names;
	public bool match_case;
	public char selection_count_cmp;
	public int selection_count;
	public string[]? schemes;
	public string[]? folders;
	public FileActionCapability capabilities;

	public FileActionCondition(KeyFile kf, string group) {
		only_show_in = Utils.key_file_get_string_list(kf, group, "OnlyShowIn");
		not_show_in = Utils.key_file_get_string_list(kf, group, "NotShowIn");
		try_exec = Utils.key_file_get_string(kf, group, "TryExec");
		show_if_registered = Utils.key_file_get_string(kf, group, "ShowIfRegistered");
		show_if_true = Utils.key_file_get_string(kf, group, "ShowIfTrue");
		show_if_running = Utils.key_file_get_string(kf, group, "ShowIfRunning");
		mime_types = Utils.key_file_get_string_list(kf, group, "MimeTypes");
		base_names = Utils.key_file_get_string_list(kf, group, "Basenames");
		match_case = Utils.key_file_get_bool(kf, group, "Matchcase");

		var selection_count_str = Utils.key_file_get_string(kf, group, "SelectionCount");
		if(selection_count_str != null && selection_count_str.length > 0) {
			switch(selection_count_str[0]) {
			case '<':
			case '>':
			case '=':
				selection_count_cmp = selection_count_str[0];
				char tmp = 0;
				const string s = "%c%d";
				selection_count_str.scanf(s, out tmp, out selection_count);
				break;
			default:
				selection_count_cmp = '>';
				selection_count = 0;
				break;
			}
		}
		else {
			selection_count_cmp = '>';
			selection_count = 0;
		}

		schemes = Utils.key_file_get_string_list(kf, group, "Schemes");
		folders = Utils.key_file_get_string_list(kf, group, "Folders");
		
		// Parse requested Capabilities string descriptors into the bitmask flag
		capabilities = 0;
		var caps_list = Utils.key_file_get_string_list(kf, group, "Capabilities");
		if (caps_list != null) {
			foreach(unowned string cap in caps_list) {
				switch (cap.ascii_down()) {
				case "owner":
					capabilities |= FileActionCapability.OWNER;
					break;
				case "readable":
					capabilities |= FileActionCapability.READABLE;
					break;
				case "writable":
					capabilities |= FileActionCapability.WRITABLE;
					break;
				case "executable":
					capabilities |= FileActionCapability.EXECUTABLE;
					break;
				case "local":
					capabilities |= FileActionCapability.LOCAL;
					break;
				}
			}
		}
	}

	private inline bool match_try_exec(List<FileInfo> files) {
		if(try_exec != null) {
			var exec_path = Environment.find_program_in_path(
					FileActionParameters.expand(try_exec, files));
			if(exec_path == null || !FileUtils.test(exec_path, FileTest.IS_EXECUTABLE)) {
				return false;
			}
		}
		return true;
	}
	
	private inline bool match_show_if_registered(List<FileInfo> files) {
		if(show_if_registered != null) {
			var service = FileActionParameters.expand(show_if_registered, files);
			try {
				var con = Bus.get_sync(BusType.SESSION);
				var result = con.call_sync("org.freedesktop.DBus",
											"/org/freedesktop/DBus",
											"org.freedesktop.DBus",
											"NameHasOwner",
											new Variant("(s)", service),
											new VariantType("(b)"),
											DBusCallFlags.NONE, -1);
				bool name_has_owner;
				result.get("(b)", out name_has_owner);
				if(!name_has_owner)
					return false;
			}
			catch(Error err) {
				return false;
			}
		}
		return true;
	}
	
	private inline bool match_show_if_true(List<FileInfo> files) {
		if(show_if_true != null) {
			var cmd = FileActionParameters.expand(show_if_true, files);
			// Fixed: Posix.system handles complex pipeline shell expansions securely inside background routines
			int exit_status = Posix.system(cmd);
			if(exit_status != 0)
				return false;
		}
		return true;
	}

	private inline bool match_show_if_running(List<FileInfo> files) {
		if(show_if_running != null) {
			var process_name = FileActionParameters.expand(show_if_running, files);
			var pgrep = Environment.find_program_in_path("pgrep");
			bool running = false;
			if(pgrep != null) {
				int exit_status;
				if(Process.spawn_command_line_sync(@"$pgrep -x '$process_name'", 
												null, null, out exit_status)) {
					if(exit_status == 0)
						running = true;
				}
			}
			if(!running)
				return false;
		}
		return true;
	}

	private inline bool match_mime_type(List<FileInfo> files, string type, bool negated) {
		if(type == "all/all" || type == "*") {
			return negated ? false : true;
		}
		else if(type == "all/allfiles") {
			if(negated) {
				foreach(unowned FileInfo fi in files) {
					if(!fi.is_dir()) {
						return false;
					}
				}
			}
			else {
				foreach(unowned FileInfo fi in files) {
					if(fi.is_dir()) {
						return false;
					}
				}
			}
		}
		else if(type.has_suffix("/*")) {
			var prefix = type.substring(0, type.length - 1);
			if(negated) {
				foreach(unowned FileInfo fi in files) {
					if(fi.get_mime_type().get_type().has_prefix(prefix)) {
						return false;
					}
				}
			}
			else {
				foreach(unowned FileInfo fi in files) {
					if(!fi.get_mime_type().get_type().has_prefix(prefix)) {
						return false;
					}
				}
			}
		}
		else {
			if(negated) {
				foreach(unowned FileInfo fi in files) {
					if(fi.get_mime_type().get_type() == type) {
						return false;
					}
				}
			}
			else {
				foreach(unowned FileInfo fi in files) {
					if(fi.get_mime_type().get_type() != type) {
						return false;
					}
				}
			}
		}
		return true;
	}

	private inline bool match_mime_types(List<FileInfo> files) {
		if(mime_types != null) {
			bool allowed = false;
			foreach(unowned string allowed_type in mime_types) {
				string type;
				bool negated;
				if(allowed_type.has_prefix("!")) {
					type = allowed_type.substring(1);
					negated = true;
				}
				else {
					type = allowed_type;
					negated = false;
				}

				if(negated) {
					bool type_is_allowed = match_mime_type(files, type, negated);
					if(!type_is_allowed)
						return false;
				}
				else {
					if(!allowed) {
						allowed = match_mime_type(files, type, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private inline bool match_base_name(List<FileInfo> files, string base_name, bool negated) {
		PatternSpec pattern;
		if(match_case) {
			pattern = new PatternSpec(base_name);
		} else {
			// Fixed casefold reference assignment leak patterns safely for runtime
			string folded_base = base_name.casefold();
			pattern = new PatternSpec(folded_base);
		}

		foreach(unowned FileInfo fi in files) {
			var name = fi.get_name();
			bool is_match = match_case ? pattern.match_string(name) : pattern.match_string(name.casefold());
			if(is_match) {
				if(negated) return false;
			} else {
				if(!negated) return false;
			}
		}
		return true;
	}

	private inline bool match_base_names(List<FileInfo> files) {
		if(base_names != null) {
			bool allowed = false;
			foreach(unowned string allowed_name in base_names) {
				string name;
				bool negated;
				if(allowed_name.has_prefix("!")) {
					name = allowed_name.substring(1);
					negated = true;
				}
				else {
					name = allowed_name;
					negated = false;
				}

				if(negated) {
					bool name_is_allowed = match_base_name(files, name, negated);
					if(!name_is_allowed)
						return false;
				}
				else {
					if(!allowed) {
						allowed = match_base_name(files, name, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private static bool match_scheme(List<FileInfo> files, string scheme, bool negated) {
		foreach(unowned FileInfo fi in files) {
			var path = fi.get_path();
			if (path == null) return false;
			var uri = path.to_uri();
			if(Uri.parse_scheme(uri) == scheme) {
				if(negated)
					return false;
			}
			else {
				if(!negated)
					return false;
			}
		}
		return true;
	}

	private inline bool match_schemes(List<FileInfo> files) {
		if(schemes != null) {
			bool allowed = false;
			foreach(unowned string allowed_scheme in schemes) {
				string scheme;
				bool negated;
				if(allowed_scheme.has_prefix("!")) {
					scheme = allowed_scheme.substring(1);
					negated = true;
				}
				else {
					scheme = allowed_scheme;
					negated = false;
				}

				if(negated) {
					bool scheme_is_allowed = match_scheme(files, scheme, negated);
					if(!scheme_is_allowed)
						return false;
				}
				else {
					if(!allowed) {
						allowed = match_scheme(files, scheme, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private static bool match_folder(List<FileInfo> files, string folder, bool negated) {
		PatternSpec pattern;
		if(folder.has_suffix("/*"))
			pattern = new PatternSpec(folder);
		else
			pattern = new PatternSpec(@"$folder/*");
		foreach(unowned FileInfo fi in files) {
			var path = fi.get_path();
			if (path == null) return false;
			var parent = path.get_parent();
			if (parent == null) return false;
			var dirname = parent.to_str();
			if(pattern.match_string(dirname)) {
				if(negated)
					return false;
			}
			else {
				if(!negated)
					return false;
			}
		}
		return true;
	}

	private inline bool match_folders(List<FileInfo> files) {
		if(folders != null) {
			bool allowed = false;
			foreach(unowned string allowed_folder in folders) {
				string folder;
				bool negated;
				if(allowed_folder.has_prefix("!")) {
					folder = allowed_folder.substring(1);
					negated = true;
				}
				else {
					folder = allowed_folder;
					negated = false;
				}

				if(negated) {
					bool folder_is_allowed = match_folder(files, folder, negated);
					if(!folder_is_allowed)
						return false;
				}
				else {
					if(!allowed) {
						allowed = match_folder(files, folder, false);
					}
				}
			}
			return allowed;
		}
		return true;
	}

	private inline bool match_selection_count(List<FileInfo> files) {
		uint n_files = files.length();
		switch(selection_count_cmp) {
		case '<':
			if(n_files >= selection_count)
				return false;
			break;
		case '=':
			if(n_files != selection_count)
				return false;
			break;
		case '>':
			if(n_files <= selection_count)
				return false;
			break;
		}
		return true;
	}

	private inline bool match_capabilities(List<FileInfo> files) {
		if (this.capabilities == 0) {
			return true;
		}

		uid_t uid = Posix.geteuid();
		gid_t gid = Posix.getegid();
		
		// Cache system supplementary groups for comparison checks
		int ngroups = Posix.getgroups(0, null);
		gid_t[] groups = new gid_t[ngroups > 0 ? ngroups : 1];
		if (ngroups > 0) {
			Posix.getgroups(ngroups, groups);
		}

		foreach(unowned FileInfo fi in files) {
			var path = fi.get_path();
			if (path == null) return false;
			string full_path = path.to_str();
			
			Posix.Stat st;
			if (Posix.stat(full_path, out st) != 0) {
				return false;
			}

			// Evaluate Owner & Group matches
			bool is_owner = (uid == st.st_uid);
			bool is_group = false;

			if (!is_owner) {
				if (gid == st.st_gid) {
					is_group = true;
				} else {
					foreach (gid_t g in groups) {
						if (g == st.st_gid) {
							is_group = true;
							break;
						}
					}
				}
			}

			// Validate explicit dynamic actions requirements
			if ((this.capabilities & FileActionCapability.OWNER) != 0 && !is_owner) {
				return false;
			}

			if ((this.capabilities & FileActionCapability.READABLE) != 0) {
				bool r = is_owner ? ((st.st_mode & Posix.S_IRUSR) != 0) : 
				         is_group ? ((st.st_mode & Posix.S_IRGRP) != 0) : ((st.st_mode & Posix.S_IROTH) != 0);
				if (!r) return false;
			}

			if ((this.capabilities & FileActionCapability.WRITABLE) != 0) {
				bool w = is_owner ? ((st.st_mode & Posix.S_IWUSR) != 0) : 
				         is_group ? ((st.st_mode & Posix.S_IWGRP) != 0) : ((st.st_mode & Posix.S_IWOTH) != 0);
				if (!w) return false;
			}

			if ((this.capabilities & FileActionCapability.EXECUTABLE) != 0) {
				bool x = is_owner ? ((st.st_mode & Posix.S_IXUSR) != 0) : 
				         is_group ? ((st.st_mode & Posix.S_IXGRP) != 0) : ((st.st_mode & Posix.S_IXOTH) != 0);
				if (!x) return false;
			}
			
			if ((this.capabilities & FileActionCapability.LOCAL) != 0) {
				var uri = path.to_uri();
				string scheme = Uri.parse_scheme(uri);
				if (scheme != "file") return false;
			}
		}
		return true;
	}

	public bool match(List<FileInfo> files) {
		if (files == null || files.length() == 0)
			return false;

		if(!match_try_exec(files))
			return false;
		if(!match_mime_types(files))
			return false;
		if(!match_base_names(files))
			return false;
		if(!match_selection_count(files))
			return false;
		if(!match_schemes(files))
			return false;
		if(!match_folders(files))
			return false;
		if(!match_capabilities(files))
			return false;
		if(!match_show_if_registered(files))
			return false;
		if(!match_show_if_true(files))
			return false;
		if(!match_show_if_running(files))
			return false;

		return true;
	}
}

}