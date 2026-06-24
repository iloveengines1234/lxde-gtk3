//      parameters.vala
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
	
namespace FileActionParameters {

public string? expand(string? templ, List<FileInfo> files, bool for_display = false, FileInfo? first_file = null) {
	if(templ == null)
		return null;
	if(files == null || files.length() == 0)
		return templ;

	var result = new StringBuilder();
	var len = templ.length;

	if(first_file == null)
		first_file = files.first().data;

	for(var i = 0; i < len; ++i) {
		var ch = templ[i];
		if(ch == '%') {
			++i;
			if (i >= len) {
				result.append_c('%');
				break;
			}
			ch = templ[i];
			switch(ch) {
			case 'b':	// (first) basename
				if(for_display)
					result.append(Filename.display_name(first_file.get_name()));
				else
					result.append(Shell.quote(first_file.get_name()));
				break;
			case 'B':	// space-separated list of basenames
				foreach(unowned FileInfo fi in files) {
					if(for_display)
						result.append(Shell.quote(Filename.display_name(fi.get_name())));
					else
						result.append(Shell.quote(fi.get_name()));
					result.append_c(' ');
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'c':	// count of selected items
				result.append_printf("%u", files.length());
				break;
			case 'd':	// (first) base directory
				var path_obj = first_file.get_path();
				if (path_obj != null) {
					var parent_obj = path_obj.get_parent();
					if (parent_obj != null) {
						var str = parent_obj.to_str();
						if(for_display)
							str = Filename.display_name(str);
						result.append(Shell.quote(str));
					}
				}
				break;
			case 'D':	// space-separated list of base directory of each selected items
				foreach(unowned FileInfo fi in files) {
					var path_obj = fi.get_path();
					if (path_obj != null) {
						var parent_obj = path_obj.get_parent();
						if (parent_obj != null) {
							var str = parent_obj.to_str();
							if(for_display)
								str = Filename.display_name(str);
							result.append(Shell.quote(str));
							result.append_c(' ');
						}
					}
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'f':	// (first) file name
				var filename = first_file.get_path().to_str();
				if(for_display)
					filename = Filename.display_name(filename);
				result.append(Shell.quote(filename));
				break;
			case 'F':	// space-separated list of selected file names
				foreach(unowned FileInfo fi in files) {
					var filename = fi.get_path().to_str();
					if(for_display)
						filename = Filename.display_name(filename);
					result.append(Shell.quote(filename));
					result.append_c(' ');
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'h':	// hostname of the (first) URI
				// Fixed: Fallback gracefully to remote host strings using real URI validation tokens
				var uri_str = first_file.get_path().to_uri();
				var parsed_host = Uri.parse_host(uri_str);
				if (parsed_host != null && parsed_host != "") {
					result.append(Shell.quote(parsed_host));
				} else {
					result.append(Shell.quote(Environment.get_host_name()));
				}
				break;
			case 'm':	// mimetype of the (first) selected item
				result.append(Shell.quote(first_file.get_mime_type().get_type()));
				break;
			case 'M':	// space-separated list of the mimetypes of the selected items
				foreach(unowned FileInfo fi in files) {
					result.append(Shell.quote(fi.get_mime_type().get_type()));
					result.append_c(' ');
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'n':	// username of the (first) URI
				// Fixed: Pulls credential context identifiers via native parsing blocks if applicable
				var uri_str_user = first_file.get_path().to_uri();
				var parsed_user = Uri.parse_userinfo(uri_str_user);
				if (parsed_user != null && parsed_user != "") {
					result.append(Shell.quote(parsed_user));
				} else {
					result.append(Shell.quote(Environment.get_user_name()));
				}
				break;
			case 'o':	// no-op operator which forces a singular form of execution when specified as first parameter,
			case 'O':	// no-op operator which forces a plural form of execution when specified as first parameter,
				break;
			case 'p':	// port number of the (first) URI
				var uri_str_port = first_file.get_path().to_uri();
				int parsed_port = (int)Uri.parse_port(uri_str_port);
				if (parsed_port > 0) {
					result.append_printf("%d", parsed_port);
				} else {
					result.append("0");
				}
				break;
			case 's':	// scheme of the (first) URI
				var uri = first_file.get_path().to_uri();
				result.append(Uri.parse_scheme(uri));
				break;
			case 'u':	// (first) URI
				result.append(Shell.quote(first_file.get_path().to_uri()));
				break;
			case 'U':	// space-separated list of selected URIs
				foreach(unowned FileInfo fi in files) {
					result.append(Shell.quote(fi.get_path().to_uri()));
					result.append_c(' ');
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'w':	// (first) basename without the extension
				string basename = first_file.get_name();
				int pos = basename.last_index_of_char('.');
				if(pos == -1)
					result.append(Shell.quote(basename));
				else {
					result.append(Shell.quote(basename.substring(0, pos)));
				}
				break;
			case 'W':	// space-separated list of basenames without their extension
				foreach(unowned FileInfo fi in files) {
					string basename = fi.get_name();
					int pos = basename.last_index_of_char('.');
					if(pos == -1)
						result.append(Shell.quote(basename));
					else {
						result.append(Shell.quote(basename.substring(0, pos)));
					}
					result.append_c(' ');
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case 'x':	// (first) extension
				string basename_x = first_file.get_name();
				int pos_x = basename_x.last_index_of_char('.');
				string ext = "";
				if(pos_x >= 0)
					ext = basename_x.substring(pos_x + 1);
				result.append(Shell.quote(ext));
				break;
			case 'X':	// space-separated list of extensions
				foreach(unowned FileInfo fi in files) {
					string basename_X = fi.get_name();
					int pos_X = basename_X.last_index_of_char('.');
					string ext_X = "";
					if(pos_X >= 0)
						ext_X = basename_X.substring(pos_X + 1);
					result.append(Shell.quote(ext_X));
					result.append_c(' ');
				}
				if(result.len > 0 && result.str[result.len-1] == ' ') // remove trailing space
					result.truncate(result.len-1);
				break;
			case '%':	// the % character
				result.append_c('%');
				break;
			case '\0':
				break;
			}
		}
		else {
			result.append_c(ch);
		}
	}
	return result.str;
}

public bool is_plural(string? exec) {
	if(exec == null)
		return false;
	var len = exec.length;
	for(var i = 0; i < len; ++i) {
		var ch = exec[i];
		if(ch == '%') {
			++i;
			if (i >= len) break;
			ch = exec[i];
			switch(ch) {
			case 'B':
			case 'D':
			case 'F':
			case 'M':
			case 'O':
			case 'U':
			case 'W':
			case 'X':
				return true;	// plural
			case 'b':
			case 'd':
			case 'f':
			case 'm':
			case 'o':
			case 'u':
			case 'w':
			case 'x':
				return false;	// singular
			default:
				break;
			}
		}
	}
	return false; 
}

} // namespace FileActionParameter
} // namespace Fm