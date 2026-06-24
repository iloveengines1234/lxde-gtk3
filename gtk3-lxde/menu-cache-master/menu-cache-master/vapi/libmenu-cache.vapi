/* libmenu-cache.vapi generated manually to align with GTK 3.24.52 modernization. */

namespace Mc {
    [CCode (cname = "MenuCacheNotifyId", has_type_id = false)]
    public struct NotifyId {
    }

    [CCode (cname = "MenuCacheReloadNotify", instance_pos = 1.1)]
    public delegate void ReloadNotify (Cache cache);

    [CCode (cname = "MenuCache", cprefix = "menu_cache_", cheader_filename = "menu-cache.h", ref_function = "menu_cache_ref", unref_function = "menu_cache_unref")]
    [Compact]
    public class Cache {
        public static void init (int flags);
        public static Cache? lookup (string menu_name);
        public static Cache? lookup_sync (string menu_name);
        
        public bool reload ();
        
        [CCode (cname = "menu_cache_dup_root_dir")]
        public CacheDir? dup_root_dir ();
        public CacheItem? item_from_path (string path);

        public NotifyId add_reload_notify (ReloadNotify func, GLib.gpointer user_data = null);
        public void remove_reload_notify (NotifyId notify_id);

        public uint32 get_desktop_env_flag (string? desktop_env);

        [CCode (cname = "menu_cache_list_all_apps")]
        public GLib.SList<CacheItem> list_all_apps ();
        [CCode (cname = "menu_cache_list_all_for_category")]
        public GLib.SList<CacheItem> list_all_for_category (string category);
        [CCode (cname = "menu_cache_list_all_for_keyword")]
        public GLib.SList<CacheItem> list_all_for_keyword (string keyword);
        public CacheItem? find_item_by_id (string id);

        /* Deprecated legacy interfaces kept for basic ABI compatibility fallback layers */
        [Version (deprecated = true, deprecated_since = "1.2.0", replacement = "dup_root_dir")]
        public unowned CacheDir get_root_dir ();
        [Version (deprecated = true, deprecated_since = "1.2.0")]
        public unowned CacheDir get_dir_from_path (string path);
    }

    [CCode (cname = "MenuCacheApp", cprefix = "menu_cache_app_", cheader_filename = "menu-cache.h")]
    [Compact]
    public class CacheApp {
        public unowned string? get_generic_name ();
        public unowned string? get_exec ();
        public unowned string? get_working_dir ();
        [CCode (cname = "menu_cache_app_get_categories")]
        public unowned string[] get_categories ();

        public uint32 get_show_flags ();
        public bool get_is_visible (uint32 de_flags);

        public bool get_use_terminal ();
        public bool get_use_sn ();
    }

    [CCode (cname = "MenuCacheDir", cprefix = "menu_cache_dir_", cheader_filename = "menu-cache.h")]
    [Compact]
    public class CacheDir {
        [CCode (cname = "menu_cache_dir_list_children")]
        public GLib.SList<CacheItem> list_children ();
        public CacheItem? find_child_by_id (string id);
        public CacheItem? find_child_by_name (string name);
        public string make_path ();
        public bool is_visible ();

        [Version (deprecated = true, deprecated_since = "1.2.0", replacement = "list_children")]
        public unowned GLib.SList get_children ();
    }

    [CCode (cname = "MenuCacheItem", cprefix = "menu_cache_item_", cheader_filename = "menu-cache.h", ref_function = "menu_cache_item_ref", unref_function = "menu_cache_item_unref")]
    [Compact]
    public class CacheItem {
        public Type get_type ();
        public unowned string? get_id ();
        public unowned string? get_name ();
        public unowned string? get_comment ();
        public unowned string? get_icon ();

        public unowned string get_file_basename ();
        public unowned string get_file_dirname ();
        public string get_file_path ();

        [CCode (cname = "menu_cache_item_dup_parent")]
        public CacheDir? dup_parent ();

        [Version (deprecated = true, deprecated_since = "1.2.0", replacement = "dup_parent")]
        public unowned CacheDir get_parent ();
    }

    [CCode (cname = "MenuCacheItemFlag", cheader_filename = "menu-cache.h", cprefix = "FLAG_", has_type_id = false)]
    public enum Item {
        USE_TERMINAL,
        USE_SN,
        IS_NODISPLAY
    }

    [CCode (cname = "MenuCacheShowFlag", cheader_filename = "menu-cache.h", cprefix = "SHOW_", has_type_id = false)]
    public enum Show {
        IN_LXDE,
        IN_GNOME,
        IN_KDE,
        IN_XFCE,
        IN_ROX,
        [CCode (cname = "N_KNOWN_DESKTOPS")]
        N_KNOWN_DESKTOPS
    }

    [CCode (cname = "MenuCacheType", cheader_filename = "menu-cache.h", cprefix = "MENU_CACHE_TYPE_", has_type_id = false)]
    public enum Type {
        NONE,
        DIR,
        APP,
        SEP
    }
}