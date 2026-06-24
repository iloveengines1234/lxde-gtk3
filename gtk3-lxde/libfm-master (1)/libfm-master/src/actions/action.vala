//      action.vala
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

private string? desktop_env; // current desktop environment
private bool actions_loaded = false; // all actions are loaded?
private HashTable<string, FileActionObject> all_actions = null; // cache all loaded actions


public enum FileActionType {
	NONE,
	ACTION,
	MENU
}

public class FileActionObject : Object {
	public FileActionType type;
	public string id;
	public string? name;
	public string? tooltip;
	public string? icon;
	public string? desc;
	public bool enabled;
	public bool hidden;
	public string? suggested_shortcut;
	public FileActionCondition condition;

	// values cached during menu generation
	public bool has_parent;

	public FileActionObject() {
	}

	public FileActionObject.from_key_file(KeyFile kf) {
		Object();
		name = Utils.key_file_get_locale_string(kf, "Desktop Entry", "Name");
		tooltip = Utils.key_file_get_locale_string(kf, "Desktop Entry", "Tooltip");
		icon = Utils.key_file_get_locale_string(kf, "Desktop Entry", "Icon");
		desc = Utils.key_file_get_locale_string(kf, "Desktop Entry", "Description");
		enabled = Utils.key_file_get_bool(kf, "Desktop Entry", "Enabled", true);
		hidden = Utils.key_file_get_bool(kf, "Desktop Entry", "Hidden", false);
		suggested_shortcut = Utils.key_file_get_string(kf, "Desktop Entry", "SuggestedShortcut");

		condition = new FileActionCondition(kf, "Desktop Entry");
	}
}


[Flags]
public enum FileActionTarget {
	NONE = 0,
	CONTEXT = 1,
	LOCATION = 1 << 1,
	TOOLBAR = 1 << 2
}

public class FileAction : FileActionObject {
	public FileActionTarget target;
	public string? toolbar_label;
	public List<FileActionProfile> profiles;
	
	// Track the file path to support reloading dynamic profiles if changed on disk
	private string file_path;
	private DateTime last_loaded;

	public FileAction(string desktop_id) {
		Object();
		id = desktop_id;
		file_path = desktop_id;
		reload_profiles();
	}

	public FileAction.from_keyfile(KeyFile kf) {
		this.from_key_file(kf); 
		type = FileActionType.ACTION;
		parse_properties(kf);
	}

	private void parse_properties(KeyFile kf) {
		if(Utils.key_file_get_bool(kf, "Desktop Entry", "TargetContext", true))
			target |= FileActionTarget.CONTEXT;
		if(Utils.key_file_get_bool(kf, "Desktop Entry", "TargetLocation"))
			target |= FileActionTarget.LOCATION;
		if(Utils.key_file_get_bool(kf, "Desktop Entry", "TargetToolbar"))
			target |= FileActionTarget.TOOLBAR;
		toolbar_label = Utils.key_file_get_locale_string(kf, "Desktop Entry", "ToolbarLabel");

		// Clear existing profiles if we are reloading dynamically
		profiles = new List<FileActionProfile>();

		string[] profile_names = Utils.key_file_get_string_list(kf, "Desktop Entry", "Profiles");
		if(profile_names != null) {
			foreach(string profile_name in profile_names) {
				profiles.prepend(new FileActionProfile(kf, profile_name.strip()));
			}
			profiles.reverse();
		}
		last_loaded = new DateTime.now_local();
	}

	private void reload_profiles() {
		var kf = new KeyFile();
		try {
			kf.load_from_file(file_path, 0);
			parse_properties(kf);
		}
		catch(KeyFileError err) {
		}
		catch(GLib.FileError err) {
		}
	}

	private void check_and_refresh_cache() {
		if (file_path == null) return;

		try {
			var file = File.new_for_path(file_path);
			var info = file.query_info(FileAttribute.TIME_MODIFIED, FileQueryInfoFlags.NONE);
			var mod_time = info.get_modification_date_time();

			// If the desktop file was modified since we last cached it, reload it dynamically
			if (mod_time.compare(last_loaded) > 0) {
				reload_profiles();
			}
		} catch (Error e) {
			// Fail gracefully if file is temporarily locked or missing
		}
	}

	public bool match(List<FileInfo> files, out unowned FileActionProfile? matched_profile) {
		matched_profile = null;
		
		// Check if the underlying .desktop file has changed before processing matches
		check_and_refresh_cache();

		if(hidden || !enabled)
			return false;

		if(!condition.match(files))
			return false;

		foreach(unowned FileActionProfile profile in profiles) {
			// Ensure the individual profile matches the current execution runtime conditions
			if(profile.match(files)) {
				matched_profile = profile;
				return true;
			}
		}
		return false;
	}
}

public class FileActionMenu : FileActionObject {
	public string[]? items_list;

	// values cached during menu generation
	public List<FileActionObject> cached_children;

	public FileActionMenu(string desktop_id) {
		Object();
		var kf = new KeyFile();
		id = desktop_id;
		try {
			kf.load_from_file(desktop_id, 0);
			this.from_keyfile(kf);
		}
		catch(KeyFileError err) {
		}
		catch(GLib.FileError err) {
		}
	}

	public FileActionMenu.from_keyfile(KeyFile kf) {
		this.from_key_file(kf); // chain up base constructor
		type = FileActionType.MENU;

		items_list = Utils.key_file_get_string_list(kf, "Desktop Entry", "ItemsList");
	}

	public bool match(List<FileInfo> files) {
		if(hidden || !enabled)
			return false;
		if(!condition.match(files))
			return false;
		return true;
	}

	// called during menu generation
	public void cache_children(List<FileInfo> files, string[] items_list) {
		foreach(unowned string item_id_prefix in items_list) {
			if(item_id_prefix[0] == '[' && item_id_prefix[item_id_prefix.length - 1] == ']') {
				// runtime dynamic item list
				string output;
				int exit_status;
				// Ported slicing behavior safely for modern Vala targets
				var command = FileActionParameters.expand(item_id_prefix.substring(1, item_id_prefix.length - 2), files);
				try {
					if(Process.spawn_command_line_sync(command, out output, null, out exit_status) && exit_status == 0) {
						string[] item_ids = output.split(";");
						cache_children(files, item_ids);
					}
				} catch (SpawnError e) {
					// Handle background thread execution crash safe for LXDE desktop sessions
				}
			}
			else if(item_id_prefix == "SEPARATOR") {
				// separator item
				cached_children.append(null);
			}
			else {
				string item_id = @"$item_id_prefix.desktop";
				FileActionObject child_action = all_actions.lookup(item_id);
				if(child_action != null) {
					child_action.has_parent = true;
					cached_children.append(child_action);
				}
			}
		}
	}
}

public class FileActionItem : Object {
	public string? name;
	public string? desc;
	public string? icon;
	public FileActionObject action;
	public unowned FileActionProfile? profile; // only used by action item
	public List<FileActionItem>? children; // only used by menu

	public static FileActionItem? new_for_action_object(FileActionObject action_obj, List<FileInfo> files) {
		FileActionItem? item = null;
		if(action_obj.type == FileActionType.MENU) {
			var menu = (FileActionMenu)action_obj;
			if(menu.match(files)) {
				item = new FileActionItem.from_menu(menu, files);
				if(item.children == null)
					item = null;
			}
		}
		else {
			var action = (FileAction)action_obj;
			unowned FileActionProfile? profile;
			if(action.match(files, out profile)) {
				item = new FileActionItem.from_action(action, profile, files);
			}
		}
		return item;
	}

	public FileActionItem.from_action(FileAction action, FileActionProfile profile, List<FileInfo> files) {
		this(action, files);
		this.profile = profile;
	}
	
	public FileActionItem.from_menu(FileActionMenu menu, List<FileInfo> files) {
		this(menu, files);
		foreach(FileActionObject action_obj in menu.cached_children) {
			if(action_obj == null) { // separator
				children.append(null);
			}
			else { // action item or menu
				FileActionItem subitem = new_for_action_object(action_obj, files);
				if(subitem != null)
					children.append(subitem);
			}
		}
	}

	private FileActionItem(FileActionObject action, List<FileInfo> files) {
		Object();
		this.action = action;
		name = FileActionParameters.expand(action.name, files, true);
		desc = FileActionParameters.expand(action.desc, files, true);
		icon = FileActionParameters.expand(action.icon, files, false);
	}

	public unowned string? get_name() {
		return name;
	}

	public unowned string? get_desc() {
		return desc;
	}

	public unowned string? get_icon() {
		return icon;
	}

	public unowned string get_id() {
		return action.id;
	}

	public FileActionTarget get_target() {
		if(action.type == FileActionType.ACTION)
			return ((FileAction)action).target;
		return FileActionTarget.NONE;
	}

	public bool is_menu() {
		return (action.type == FileActionType.MENU);
	}

	public bool is_action() {
		return (action.type == FileActionType.ACTION);
	}

	public bool launch(AppLaunchContext ctx, List<FileInfo> files, out string? output) {
		output = null;
		if(action.type == FileActionType.ACTION) {
			if(profile != null) {
				profile.launch(ctx, files, out output);
			}
			return true;
		}
		return false;
	}

	public unowned List<FileActionItem>? get_sub_items() {
		if(action != null && action.type == FileActionType.MENU)
			return children;
		return null;
	}
}


private void load_actions_from_dir(string dirname, string? id_prefix) {
	try {
		var dir = Dir.open(dirname, 0);
		if(dir != null) {
			string? name;
			var kf = new KeyFile();
			while ((name = dir.read_name()) != null) {
				var full_path = GLib.Path.build_filename(dirname, name);

				if(FileUtils.test(full_path, FileTest.IS_DIR)) {
					load_actions_from_dir(full_path, id_prefix != null ? @"$id_prefix-$name" : name);
				}
				else if (name.has_suffix(".desktop")) {
					string id = id_prefix != null ? @"$id_prefix-$name" : name;
					if(all_actions.lookup(id) == null) {
						if(kf.load_from_file(full_path, 0)) {
							string? type = Utils.key_file_get_string(kf, "Desktop Entry", "Type");
							FileActionObject? action = null;
							if(type == null || type == "Action") {
								action = new FileAction.from_keyfile(kf);
							}
							else if(type == "Menu") {
								action = new FileActionMenu.from_keyfile(kf);
							}
							else {
								continue;
							}
							action.id = id;
							all_actions.insert(id, action);
						}
					}
				}
			}
		}
	}
	catch(GLib.FileError err) {
	}
}


public List<FileActionItem>? get_actions_for_files(List<Fm.FileInfo> files) {
	if(!actions_loaded)
		load_all_actions();

	var action_it = HashTableIter<string, FileActionObject>(all_actions);
	unowned string k;
	unowned FileActionObject action_obj;
	while(action_it.next(out k, out action_obj)) {
		if(action_obj.type == FileActionType.MENU) {
			FileActionMenu menu = (FileActionMenu)action_obj;
			menu.cache_children(files, menu.items_list);
		}
	}

	var items = new List<FileActionItem>();

	action_it = HashTableIter<string, FileActionObject>(all_actions);
	while(action_it.next(out k, out action_obj)) {
		if(action_obj.has_parent == false) {
			FileActionItem? item = FileActionItem.new_for_action_object(action_obj, files);
			if(item != null)
				items.append(item);
		}
	}

	action_it = HashTableIter<string, FileActionObject>(all_actions);
	while(action_it.next(out k, out action_obj)) {
		action_obj.has_parent = false;
		if(action_obj.type == FileActionType.MENU) {
			FileActionMenu menu = (FileActionMenu)action_obj;
			menu.cached_children = null;
		}
	}
	return items;
}

private void load_all_actions() {
	all_actions.remove_all();
	unowned string[] dirs = Environment.get_system_data_dirs();
	foreach(unowned string dir in dirs) {
		load_actions_from_dir(GLib.Path.build_filename(dir, "file-manager/actions"), null);
	}
	load_actions_from_dir(GLib.Path.build_filename(Environment.get_user_data_dir(), "file-manager/actions"), null);
	actions_loaded = true;
}

public void file_actions_set_desktop_env(string env) {
	desktop_env = env;
}

}

namespace _Fm {

public void file_actions_init() {
	Fm.all_actions = new HashTable<string, Fm.FileActionObject>(str_hash, str_equal);
}

public void file_actions_finalize() {
	Fm.all_actions = null;
}

}