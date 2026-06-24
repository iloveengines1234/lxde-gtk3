/*
 *      Copyright 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of libmenu-cache package and created program
 *      should be not used without the library.
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef MENU_TAGS_H
#define MENU_TAGS_H

#include <libfm/fm-extra.h>
#include <menu-cache.h>
#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* --- XML Tag Declarations --- */
extern FmXmlFileTag menuTag_Menu;
extern FmXmlFileTag menuTag_AppDir;
extern FmXmlFileTag menuTag_DefaultAppDirs;
extern FmXmlFileTag menuTag_DirectoryDir;
extern FmXmlFileTag menuTag_DefaultDirectoryDirs;
extern FmXmlFileTag menuTag_Include;
extern FmXmlFileTag menuTag_Exclude;
extern FmXmlFileTag menuTag_Filename;
extern FmXmlFileTag menuTag_Or;
extern FmXmlFileTag menuTag_And;
extern FmXmlFileTag menuTag_Not;
extern FmXmlFileTag menuTag_Category;
extern FmXmlFileTag menuTag_All;
extern FmXmlFileTag menuTag_LegacyDir;
extern FmXmlFileTag menuTag_Deleted;
extern FmXmlFileTag menuTag_NotDeleted;
extern FmXmlFileTag menuTag_Directory;
extern FmXmlFileTag menuTag_Name;
extern FmXmlFileTag menuTag_OnlyUnallocated;
extern FmXmlFileTag menuTag_NotOnlyUnallocated;
extern FmXmlFileTag menuTag_MergeFile;
extern FmXmlFileTag menuTag_MergeDir;
extern FmXmlFileTag menuTag_DefaultMergeDirs;
extern FmXmlFileTag menuTag_KDELegacyDirs;
extern FmXmlFileTag menuTag_Move;
extern FmXmlFileTag menuTag_Old;
extern FmXmlFileTag menuTag_New;
extern FmXmlFileTag menuTag_Layout;
extern FmXmlFileTag menuTag_DefaultLayout;
extern FmXmlFileTag menuTag_Menuname;
extern FmXmlFileTag menuTag_Separator;
extern FmXmlFileTag menuTag_Merge;

/* --- Merge Types (fixed ordering) --- */
typedef enum {
    MERGE_NONE = 0,
    MERGE_FILES,
    MERGE_MENUS,
    MERGE_ALL,
    MERGE_FILES_MENUS,
    MERGE_MENUS_FILES
} MenuMergeType;

/* --- Layout Structure (bitfields optimized for uniform packing) --- */
typedef struct {
    guint type : 3;                     /* MENU_CACHE_TYPE_DIR unsigned storage */
    guint only_unallocated : 1;
    guint is_set : 1;
    guint show_empty : 1;
    guint allow_inline : 1;
    guint inline_header : 1;
    guint inline_alias : 1;
    guint inline_header_is_set : 1;
    guint inline_alias_is_set : 1;
    guint inline_limit_is_set : 1;
    guint nodisplay : 1;
    GList *items;                       /* Menuname / Filename / Separator / Merge */
    int inline_limit;
} MenuLayout;

/* Menuname item */
typedef struct {
    MenuLayout layout;
    char *name;
} MenuMenuname;

/* Filename item */
typedef struct {
    guint type : 3;                     /* MENU_CACHE_TYPE_APP */
    char *id;
} MenuFilename;

/* Separator item */
typedef struct {
    guint type : 3;                     /* MENU_CACHE_TYPE_SEP */
} MenuSep;

/* Merge item */
typedef struct {
    guint type : 3;                     /* MENU_CACHE_TYPE_NONE */
    MenuMergeType merge_type;
} MenuMerge;

/* Menu item */
typedef struct {
    MenuLayout layout;
    char *name;
    char *key;
    GList *id;                          /* <Directory> list */
    GList *children;                    /* MenuApp / MenuMenu / MenuSep / MenuRule */
    char *title;
    char *comment;
    char *icon;
    const char *dir;
} MenuMenu;

/* File item */
typedef struct {
    guint type : 3;                     /* MENU_CACHE_TYPE_APP */
    guint excluded : 1;
    guint allocated : 1;
    guint matched : 1;
    guint use_terminal : 1;
    guint use_notification : 1;
    guint hidden : 1;
    GList *dirs;
    GList *menus;
    char *filename;
    char *key;
    char *id;
    char *title;
    char *comment;
    char *icon;
    char *generic_name;
    char *exec;
    char *try_exec;
    char *wd;
    const char **categories;
    const char **keywords;
    const char **show_in;
    const char **hide_in;
} MenuApp;

/* Rule placeholder */
typedef struct {
    guint type : 3;                     /* MENU_CACHE_TYPE_NONE */
    FmXmlFileItem *rule;
} MenuRule;

/* --- Environment & States --- */
extern char **languages;
extern GSList *MenuFiles;
extern GSList *MenuDirs;
extern GSList *AppDirs;
extern GSList *DirDirs;

/* --- Public API --- */
MenuMenu *get_merged_menu(const char *file, FmXmlFile **xmlfile, GError **error);
gboolean  save_menu_cache(MenuMenu *layout, const char *menuname, const char *file, gboolean with_hidden);
void      _free_layout_items(GList *data);

/* --- Logging (Namespaced to prevent compilation collisions) --- */
extern gint verbose;

#define MC_DBG(fmt, ...)      do { if (verbose)     g_debug(fmt, ##__VA_ARGS__); } while (0)
#define MC_VDBG(fmt, ...)     do { if (verbose > 1) g_debug(fmt, ##__VA_ARGS__); } while (0)
#define MC_VVDBG(fmt, ...)    do { if (verbose > 2) g_debug(fmt, ##__VA_ARGS__); } while (0)

G_END_DECLS

#endif /* MENU_TAGS_H */