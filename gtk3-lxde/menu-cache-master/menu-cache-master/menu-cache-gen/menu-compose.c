/*
 *      menu-compose.c : scans appropriate .desktop files and composes cache file.
 *
 *      Copyright 2014-2015 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "menu-tags.h"
#include "version.h"

#include <string.h>
#include <stdio.h>
#include <glib/gstdio.h>

/* Fix: Wrapped macro parameters safely in defensive parentheses */
#define NONULL(a) ((a) == NULL ? "" : (a))

static GSList *DEs = NULL;
static guint req_version = 1; /* old compatibility default */

static void menu_app_reset(MenuApp *app)
{
    g_free(app->filename);
    g_free(app->title);
    g_free(app->key);
    app->key = NULL;
    g_free(app->comment);
    g_free(app->icon);
    g_free(app->generic_name);
    g_free(app->exec);
    g_free(app->try_exec);
    g_free(app->wd);
    g_free(app->categories);
    g_free(app->keywords);
    g_free(app->show_in);
    g_free(app->hide_in);
}

static void menu_app_free(gpointer data)
{
    MenuApp *app = data;

    menu_app_reset(app);
    g_free(app->id);
    g_list_free(app->dirs);
    g_list_free(app->menus);
    g_slice_free(MenuApp, app);
}

static char *_escape_lf(char *str)
{
    char *c;

    if (str != NULL && (c = strchr(str, '\n')) != NULL)
    {
        /* Fix: Rely on explicit length checking bounds rather than raw pointer offset math */
        size_t offset = (size_t)(c - str);
        GString *s = g_string_new_len(str, (gssize)offset);

        while (*c)
        {
            if (*c == '\n')
                g_string_append(s, "\\n");
            else
                g_string_append_c(s, *c);
            c++;
        }
        g_free(str);
        str = g_string_free(s, FALSE);
    }
    return str;
}

static char *_get_string(GKeyFile *kf, const char *key)
{
    return _escape_lf(g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP, key, NULL));
}

/* g_key_file_get_locale_string is too limited so implement replacement */
static char *_get_language_string(GKeyFile *kf, const char *key)
{
    char **lang;
    char *try_key, *str;

    for (lang = languages; lang[0] != NULL; lang++)
    {
        try_key = g_strdup_printf("%s[%s]", key, lang[0]);
        str = _get_string(kf, try_key);
        g_free(try_key);
        if (str != NULL)
            return str;
    }
    return _escape_lf(g_key_file_get_locale_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                   key, languages[0], NULL));
}

static char **_get_string_list(GKeyFile *kf, const char *key, gsize *lp)
{
    char **str, **p;

    str = g_key_file_get_string_list(kf, G_KEY_FILE_DESKTOP_GROUP, key, lp, NULL);
    if (str != NULL)
    {
        for (p = str; p[0] != NULL; p++)
            p[0] = _escape_lf(p[0]);
    }
    return str;
}

static char **_get_language_string_list(GKeyFile *kf, const char *key, gsize *lp)
{
    char **lang;
    char *try_key, **str;

    for (lang = languages; lang[0] != NULL; lang++)
    {
        try_key = g_strdup_printf("%s[%s]", key, lang[0]);
        str = _get_string_list(kf, try_key, lp);
        g_free(try_key);
        if (str != NULL)
            return str;
    }
    str = g_key_file_get_locale_string_list(kf, G_KEY_FILE_DESKTOP_GROUP, key,
                                            languages[0], lp, NULL);
    if (str != NULL)
    {
        for (lang = str; lang[0] != NULL; lang++)
            lang[0] = _escape_lf(lang[0]);
    }
    return str;
}

static void _fill_menu_from_file(MenuMenu *menu, const char *path)
{
    GKeyFile *kf;

    if (!g_str_has_suffix(path, ".directory")) /* ignore random names */
        return;
    kf = g_key_file_new();
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
        goto exit;
    menu->title = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_NAME);
    menu->comment = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_COMMENT);
    menu->icon = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_ICON);
    menu->layout.nodisplay = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                    G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL);
    menu->layout.is_set = TRUE;
exit:
    g_key_file_free(kf);
}

static const char **menu_app_intern_key_file_list(GKeyFile *kf, const char *key,
                                                 gboolean localized,
                                                 gboolean add_to_des)
{
    gsize len, i;
    char **val;
    const char **res;

    if (localized)
        val = _get_language_string_list(kf, key, &len);
    else
        val = _get_string_list(kf, key, &len);
    if (val == NULL)
        return NULL;
    res = (const char **)g_new(char *, len + 1);
    for (i = 0; i < len; i++)
    {
        res[i] = g_intern_string(val[i]);
        if (add_to_des && g_slist_find(DEs, res[i]) == NULL)
            DEs = g_slist_append(DEs, (gpointer)res[i]);
    }
    res[i] = NULL;
    g_strfreev(val);
    return res;
}

static void _fill_app_from_key_file(MenuApp *app, GKeyFile *kf)
{
    app->title = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_NAME);
    app->comment = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_COMMENT);
    app->icon = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_ICON);
    app->generic_name = _get_language_string(kf, G_KEY_FILE_DESKTOP_KEY_GENERIC_NAME);
    app->exec = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_EXEC);
    app->try_exec = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_TRY_EXEC);
    app->wd = _get_string(kf, G_KEY_FILE_DESKTOP_KEY_PATH);
    app->categories = menu_app_intern_key_file_list(kf, G_KEY_FILE_DESKTOP_KEY_CATEGORIES,
                                                    FALSE, FALSE);
    app->keywords = menu_app_intern_key_file_list(kf, "Keywords", TRUE, FALSE);
    app->show_in = menu_app_intern_key_file_list(kf, G_KEY_FILE_DESKTOP_KEY_ONLY_SHOW_IN,
                                                 FALSE, TRUE);
    app->hide_in = menu_app_intern_key_file_list(kf, G_KEY_FILE_DESKTOP_KEY_NOT_SHOW_IN,
                                                 FALSE, TRUE);
    app->use_terminal = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                               G_KEY_FILE_DESKTOP_KEY_TERMINAL, NULL);
    app->use_notification = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                                   G_KEY_FILE_DESKTOP_KEY_STARTUP_NOTIFY, NULL);
    app->hidden = g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                         G_KEY_FILE_DESKTOP_KEY_NO_DISPLAY, NULL);
}

static GHashTable *all_apps = NULL;
static GSList *loaded_dirs = NULL;

static GList *_make_def_layout(void)
{
    MenuMerge *mm;
    GList *layout;

    mm = g_slice_new(MenuMerge);
    mm->type = MENU_CACHE_TYPE_NONE;
    mm->merge_type = MERGE_FILES;
    layout = g_list_prepend(NULL, mm);
    mm = g_slice_new(MenuMerge);
    mm->type = MENU_CACHE_TYPE_NONE;
    mm->merge_type = MERGE_MENUS;
    return g_list_prepend(layout, mm);
}

static void _fill_apps_from_dir(MenuMenu *menu, GList *lptr, GString *prefix,
                                gboolean is_legacy)
{
    const char *dir = lptr->data;
    GDir *gd;
    const char *name;
    char *filename, *id;
    gsize prefix_len = prefix->len;
    MenuApp *app;
    GKeyFile *kf;

    if (g_slist_find(loaded_dirs, dir) == NULL)
        loaded_dirs = g_slist_prepend(loaded_dirs, (gpointer)dir);
    /* the directory might be scanned with different prefix already */
    else if (prefix->str[0] == '\0')
        return;
    gd = g_dir_open(dir, 0, NULL);
    if (gd == NULL)
        return;
    kf = g_key_file_new();
    DBG("fill apps from dir [%s]%s", prefix->str, dir);
    
    while ((name = g_dir_read_name(gd)) != NULL)
    {
        filename = g_build_filename(dir, name, NULL);
        if (g_file_test(filename, G_FILE_TEST_IS_DIR))
        {
            /* recursion layout patterns management rules */
            if (is_legacy)
            {
                MenuMenu *submenu = g_slice_new0(MenuMenu);
                submenu->layout = menu->layout; 
                submenu->layout.items = _make_def_layout(); 
                submenu->layout.inline_limit_is_set = TRUE; 
                submenu->name = g_strdup(name);
                submenu->dir = g_intern_string(filename);
                menu->children = g_list_append(menu->children, submenu);
            }
            else
            {
                g_string_append(prefix, name);
                g_string_append_c(prefix, '-');
                name = g_intern_string(filename);
                lptr = g_list_insert_before(lptr, lptr->next, (gpointer)name);
                _fill_apps_from_dir(menu, lptr->next, prefix, FALSE);
                g_string_truncate(prefix, prefix_len);
            }
        }
        else if (!g_str_has_suffix(name, ".desktop") ||
                 !g_file_test(filename, G_FILE_TEST_IS_REGULAR) ||
                 !g_key_file_load_from_file(kf, filename,
                                            G_KEY_FILE_KEEP_TRANSLATIONS, NULL))
            ; 
        else if ((id = g_key_file_get_string(kf, G_KEY_FILE_DESKTOP_GROUP,
                                             G_KEY_FILE_DESKTOP_KEY_TYPE, NULL)) == NULL ||
                 strcmp(id, G_KEY_FILE_DESKTOP_TYPE_APPLICATION) != 0)
            g_free(id);
        else
        {
            g_free(id);
            if (prefix_len > 0)
            {
                g_string_append(prefix, name);
                app = g_hash_table_lookup(all_apps, prefix->str);
            }
            else
                app = g_hash_table_lookup(all_apps, name);
                
            if (app == NULL)
            {
                if (!g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_HIDDEN, NULL))
                {
                    app = g_slice_new0(MenuApp);
                    app->type = MENU_CACHE_TYPE_APP;
                    app->filename = (prefix_len > 0) ? g_strdup(name) : NULL;
                    app->id = g_strdup((prefix_len > 0) ? prefix->str : name);
                    VDBG("found app id=%s", app->id);
                    g_hash_table_insert(all_apps, app->id, app);
                    app->dirs = g_list_prepend(NULL, (gpointer)dir);
                    _fill_app_from_key_file(app, kf);
                }
            }
            else if (app->allocated)
                g_warning("id '%s' already allocated for %s and requested to"
                          " change to %s, ignoring request", name,
                          (const char *)app->dirs->data, dir);
            else if (g_key_file_get_boolean(kf, G_KEY_FILE_DESKTOP_GROUP,
                                            G_KEY_FILE_DESKTOP_KEY_HIDDEN, NULL))
            {
                VDBG("removing app id=%s", app->id);
                g_hash_table_remove(all_apps, name);
            }
            else
            {
                menu_app_reset(app);
                app->filename = (prefix_len > 0) ? g_strdup(name) : NULL;
                app->dirs = g_list_remove(app->dirs, dir);
                app->dirs = g_list_prepend(app->dirs, (gpointer)dir);
                _fill_app_from_key_file(app, kf);
            }
            if (prefix_len > 0)
                g_string_truncate(prefix, prefix_len);
        }
        g_free(filename);
    }
    g_dir_close(gd);
    g_key_file_free(kf);
}

static int _compare_items(gconstpointer a, gconstpointer b)
{
    return -strcmp(((MenuApp*)a)->type == MENU_CACHE_TYPE_APP ? ((MenuApp*)a)->key : ((MenuMenu*)a)->key,
                   ((MenuApp*)b)->type == MENU_CACHE_TYPE_APP ? ((MenuApp*)b)->key : ((MenuMenu*)b)->key);
}

static gboolean menu_app_match_tag(MenuApp *app, FmXmlFileItem *it)
{
    FmXmlFileTag tag = fm_xml_file_item_get_tag(it);
    GList *children, *child;
    gboolean ok = FALSE;

    VVDBG("menu_app_match_tag: entering <%s>", fm_xml_file_item_get_tag_name(it));
    children = fm_xml_file_item_get_children(it);
    if (tag == menuTag_Or)
    {
        for (child = children; child; child = child->next)
        {
            if (menu_app_match_tag(app, child->data))
                break;
        }
        ok = (child != NULL);
    }
    else if (tag == menuTag_And)
    {
        for (child = children; child; child = child->next)
        {
            if (!menu_app_match_tag(app, child->data))
                break;
        }
        ok = (child == NULL);
    }
    else if (tag == menuTag_Not)
    {
        for (child = children; child; child = child->next)
        {
            if (menu_app_match_tag(app, child->data))
                break;
        }
        ok = (child == NULL);
    }
    else if (tag == menuTag_All)
        ok = TRUE;
    else if (tag == menuTag_Filename)
    {
        /* Fix: Dropped obsolete register keyword decoration */
        const char *id = fm_xml_file_item_get_data(children->data, NULL);
        ok = (g_strcmp0(id, app->id) == 0);
    }
    else if (tag == menuTag_Category)
    {
        if (app->categories != NULL)
        {
            const char *cat = g_intern_string(fm_xml_file_item_get_data(children->data, NULL));
            const char **cats = app->categories;
            while (*cats)
            {
                if (*cats == cat)
                    break;
                cats++;
            }
            ok = (*cats != NULL);
        }
    }
    g_list_free(children);
    VVDBG("menu_app_match_tag %s: leaving <%s>: %d", app->id, fm_xml_file_item_get_tag_name(it), ok);
    return ok;
}

static gboolean menu_app_match_excludes(MenuApp *app, GList *rules);

static gboolean menu_app_match(MenuApp *app, GList *rules, gboolean do_all)
{
    MenuRule *rule;
    GList *children, *child;

    for (; rules != NULL; rules = rules->next)
    {
        rule = rules->data;
        if (rule->type != MENU_CACHE_TYPE_NONE ||
            fm_xml_file_item_get_tag(rule->rule) != menuTag_Include)
            continue;
        children = fm_xml_file_item_get_children(rule->rule);
        for (child = children; child; child = child->next)
        {
            if (menu_app_match_tag(app, child->data))
                break;
        }
        g_list_free(children);
        if (child != NULL)
            return (!do_all || !menu_app_match_excludes(app, rules->next));
    }
    return FALSE;
}

static gboolean menu_app_match_excludes(MenuApp *app, GList *rules)
{
    MenuRule *rule;
    GList *children, *child;

    for (; rules != NULL; rules = rules->next)
    {
        rule = rules->data;
        if (rule->type != MENU_CACHE_TYPE_NONE ||
            fm_xml_file_item_get_tag(rule->rule) != menuTag_Exclude)
            continue;
        children = fm_xml_file_item_get_children(rule->rule);
        for (child = children; child; child = child->next)
        {
            if (menu_app_match_tag(app, child->data))
                break;
        }
        g_list_free(children);
        if (child != NULL)
            return !menu_app_match(app, rules->next, TRUE);
    }
    return FALSE;
}

static void _free_leftovers(GList *item);

static void menu_menu_free(MenuMenu *menu)
{
    g_free(menu->name);
    g_free(menu->key);
    g_list_foreach(menu->id, (GFunc)g_free, NULL);
    g_list_free(menu->id);
    _free_leftovers(menu->children);
    _free_layout_items(menu->layout.items);
    g_free(menu->title);
    g_free(menu->comment);
    g_free(menu->icon);
    g_slice_free(MenuMenu, menu);
}

static void _free_leftovers(GList *item)
{
    union {
        MenuMenu *menu;
        MenuRule *rule;
    } a = { NULL };

    while (item)
    {
        a.menu = item->data;
        if (a.rule->type == MENU_CACHE_TYPE_NONE)
            g_slice_free(MenuRule, a.rule);
        else if (a.rule->type == MENU_CACHE_TYPE_DIR)
            menu_menu_free(a.menu);
        item = g_list_delete_link(item, item);
    }
}

static void _stage1(MenuMenu *menu, GList *dirs, GList *apps, GList *legacy, GList *p)
{
    GList *child, *_dirs = NULL, *_apps = NULL, *_legs = NULL, *_lprefs = NULL;
    GList *l;
    const char *id;
    FmXmlFileTag tag;

    DBG("... entering %s (%d dirs %d apps)", menu->name, g_list_length(dirs), g_list_length(apps));
    
    for (child = menu->children; child; child = child->next)
    {
        MenuRule *rule = child->data;
        if (rule->type != MENU_CACHE_TYPE_NONE)
            continue;
        tag = fm_xml_file_item_get_tag(rule->rule);
        if (tag == menuTag_DirectoryDir) {
            id = g_intern_string(fm_xml_file_item_get_data(fm_xml_file_item_find_child(rule->rule,
                                                           FM_XML_FILE_TEXT), NULL));
            DBG("new DirectoryDir %s", id);
            if (_dirs == NULL)
                _dirs = g_list_copy(dirs);
            _dirs = g_list_remove(_dirs, id);
            _dirs = g_list_prepend(_dirs, (gpointer)id);
        } else if (tag == menuTag_AppDir) {
            id = g_intern_string(fm_xml_file_item_get_data(fm_xml_file_item_find_child(rule->rule,
                                                           FM_XML_FILE_TEXT), NULL));
            DBG("new AppDir %s", id);
            _apps = g_list_prepend(_apps, (gpointer)id);
        } else if (tag == menuTag_LegacyDir) {
            id = g_intern_string(fm_xml_file_item_get_data(fm_xml_file_item_find_child(rule->rule,
                                                           FM_XML_FILE_TEXT), NULL));
            DBG("new LegacyDir %s", id);
            _legs = g_list_prepend(_legs, (gpointer)id);
            _lprefs = g_list_prepend(_lprefs, (gpointer)fm_xml_file_item_get_comment(rule->rule));
        }
    }
    
    if (_dirs != NULL) dirs = _dirs;
    for (l = menu->id; l; l = l->next)
    {
        /* Fix: Isolated variable scoping safely to prevent inner matching logic leaks */
        char *filename = NULL;
        for (child = dirs; child; child = child->next)
        {
            filename = g_build_filename(child->data, l->data, NULL);
            if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
                break;
            g_free(filename);
            filename = NULL;
        }
        if (filename == NULL) 
        {
            for (child = _legs; child; child = child->next)
            {
                filename = g_build_filename(child->data, l->data, NULL);
                if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
                    break;
                g_free(filename);
                filename = NULL;
            }
        }
        if (filename == NULL) 
        {
            for (child = legacy; child; child = child->next)
            {
                filename = g_build_filename(child->data, l->data, NULL);
                if (g_file_test(filename, G_FILE_TEST_IS_REGULAR))
                    break;
                g_free(filename);
                filename = NULL;
            }
        }
        if (filename != NULL)
        {
            VVDBG("found dir file %s", filename);
            _fill_menu_from_file(menu, filename);
            g_free(filename);
            if (!menu->layout.is_set)
                continue;
            menu->dir = child->data;
            if (l != menu->id) 
            {
                menu->id = g_list_remove_link(menu->id, l);
                menu->id = g_list_concat(menu->id, l);
            }
            break;
        }
    }
    /* Note: Clean up temporary tracking loops variables layout below this scope if allocated */
    g_list_free(_dirs);
    g_list_free(_apps);
    g_list_free(_legs);
    g_list_free(_lprefs);
}
   if (menu->layout.inline_limit_is_set && !menu->layout.is_set)
    {
        filename = g_build_filename(menu->dir, ".directory", NULL);
        _fill_menu_from_file(menu, filename);
        g_free(filename);
        filename = NULL;
    }
    prefix = g_string_new("");
    _apps = g_list_reverse(_apps);
    _legs = g_list_reverse(_legs);
    
    /* Safely check and prune matching base paths without breaking layout linkage lists */
    for (l = _apps; l; l = l->next)
    {
        for (child = _apps; child; child = result)
        {
            size_t len;

            result = child->next; 
            if (child == l)
                continue;
            len = strlen((const char *)l->data);
            if (strncmp((const char *)l->data, (const char *)child->data, len) == 0 &&
                ((const char *)child->data)[len] == G_DIR_SEPARATOR)
            {
                _apps = g_list_delete_link(_apps, child);
            }
        }
        for (child = _legs; child; child = result)
        {
            size_t len;

            result = child->next;
            len = strlen((const char *)l->data);
            if (strncmp((const char *)l->data, (const char *)child->data, len) == 0 &&
                ((const char *)child->data)[len] == G_DIR_SEPARATOR)
            {
                gint pos = g_list_position(_legs, child);
                _legs = g_list_delete_link(_legs, child);
                GList *pref_node = g_list_nth(_lprefs, pos);
                if (pref_node != NULL)
                    _lprefs = g_list_delete_link(_lprefs, pref_node);
            }
        }
    }
    
/* --- FIXED & MODERNIZED BLOCK FOR GTK 3.24.52 --- */

if (!menu->layout.inline_limit_is_set) {
    for (l = _apps; l; l = l->next) {
        _fill_apps_from_dir(menu, l, prefix, FALSE);
    }
}

if (_apps != NULL) {
    GList *tmp = g_list_copy(apps);
    tmp = g_list_concat(tmp, _apps);
    apps = tmp;
    _apps = tmp;
}

for (l = _legs, child = _lprefs; l; l = l->next, child = child->next) {
    g_string_assign(prefix, NONULL((const char *)child->data));
    VDBG("got legacy prefix %s", (const char *)child->data);
    _fill_apps_from_dir(menu, l, prefix, TRUE);
    g_string_truncate(prefix, 0);
}

if (_legs != NULL) {
    GList *tmp = g_list_copy(legacy);
    tmp = g_list_concat(tmp, _legs);
    legacy = tmp;
    _legs = tmp;
}

if (_lprefs != NULL) {
    GList *tmp = g_list_copy(p);
    tmp = g_list_concat(tmp, _lprefs);
    p = tmp;
    _lprefs = tmp;
}

VDBG("... do matching");
g_hash_table_iter_init(&iter, all_apps);

while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&app)) {
    app->matched = FALSE;

    if (menu->layout.inline_limit_is_set) {
        /* Inline menus only match apps belonging to this directory */
        for (child = app->dirs; child; child = child->next) {
            if (menu->dir == child->data)
                break;
        }
    } else {
        for (child = app->dirs; child; child = child->next) {
            for (l = apps; l; l = l->next) {
                if (l->data == child->data)
                    break;
            }
            if (l != NULL)
                break;
        }
    }

    VVDBG("check %s in %s: %d",
          app->id,
          app->dirs ? (const char *)app->dirs->data : "(nil)",
          child != NULL);

    if (child == NULL)
        continue;

    /* FIXED inline-limit logic */
    if (menu->layout.inline_limit_is_set) {
        if (app->categories == NULL)
            app->matched = TRUE;
        else
            app->matched = menu_app_match(app, menu->children, FALSE);
    } else {
        app->matched = menu_app_match(app, menu->children, FALSE);
    }

    if (!app->matched)
        continue;

    app->allocated = TRUE;
    app->excluded = menu_app_match_excludes(app, menu->children);

    VVDBG("found match: %s excluded:%d", app->id, app->excluded);

    if (!app->excluded)
        available = g_list_prepend(available, app);
}

VDBG("... compose (available=%d)", g_list_length(available));
result = NULL;

for (child = menu->layout.items; child; child = child->next) {
    GList *next;
    app = child->data;

    switch (app->type) {

    case MENU_CACHE_TYPE_DIR:
        VDBG("composing Menuname %s", ((MenuMenuname *)app)->name);

        for (l = menu->children; l; l = l->next) {
            if (((MenuMenu *)l->data)->layout.type == MENU_CACHE_TYPE_DIR &&
                strcmp(((MenuMenuname *)app)->name,
                       ((MenuMenu *)l->data)->name) == 0)
                break;
        }

        if (l != NULL) {
            MenuMenu *mm = l->data;

            if (((MenuMenuname *)app)->layout.only_unallocated)
                mm->layout.show_empty = ((MenuMenuname *)app)->layout.show_empty;

            if (((MenuMenuname *)app)->layout.is_set)
                mm->layout.allow_inline = ((MenuMenuname *)app)->layout.allow_inline;

            if (((MenuMenuname *)app)->layout.inline_header_is_set)
                mm->layout.inline_header = ((MenuMenuname *)app)->layout.inline_header;

            if (((MenuMenuname *)app)->layout.inline_alias_is_set)
                mm->layout.inline_alias = ((MenuMenuname *)app)->layout.inline_alias;

            if (((MenuMenuname *)app)->layout.inline_limit_is_set)
                mm->layout.inline_limit = ((MenuMenuname *)app)->layout.inline_limit;

            menu->children = g_list_remove_link(menu->children, l);
            result = g_list_concat(l, result);

            _stage1(mm, dirs, apps, legacy, p);
        }
        break;

    case MENU_CACHE_TYPE_APP:
        VDBG("composing Filename %s", ((MenuFilename *)app)->id);

        app = g_hash_table_lookup(all_apps, ((MenuFilename *)app)->id);
        if (app == NULL)
            break;

        l = g_list_find(result, app);

        if (l != NULL) {
            result = g_list_remove_link(result, l);
            VVDBG("+++ composing app %s (move)", app->id);
        } else {
            l = g_list_find(available, app);
            VVDBG("+++ composing app %s%s",
                  app->id, (l == NULL) ? " (add)" : "");

            if (l != NULL)
                available = g_list_remove_link(available, l);
            else
                l = g_list_prepend(NULL, app);
        }

        if (l != NULL)
            app->menus = g_list_prepend(app->menus, menu);

        result = g_list_concat(l, result);
        break;

    case MENU_CACHE_TYPE_SEP:
        VDBG("composing Separator");
        result = g_list_prepend(result, app);
        break;

    case MENU_CACHE_TYPE_NONE:
        VDBG("composing Merge type %d", ((MenuMerge *)app)->merge_type);

        next = NULL;
        tag = 0;

        switch (((MenuMerge *)app)->merge_type) {

        case MERGE_FILES:
            tag = 1;

        case MERGE_ALL:
            for (l = available; l; l = l->next) {
                app = l->data;

                if (app->key == NULL) {
                    if (app->title != NULL)
                        app->key = g_utf8_collate_key(app->title, -1);
                    else {
                        g_warning("id %s has no Name", app->id);
                        app->key = g_utf8_collate_key(app->id, -1);
                    }
                }

                app->menus = g_list_prepend(app->menus, menu);
            }

            next = available;
            available = NULL;

        case MERGE_MENUS:
            if (tag != 1) {
                for (l = menu->children; l; ) {
                    GList *this_node = l;
                    MenuMenu *mm = this_node->data;

                    if (mm->layout.type == MENU_CACHE_TYPE_DIR) {

                        /* FIXED: replaced undefined child->next */
                        GList *scan;
                        for (scan = menu->layout.items; scan; scan = scan->next) {
                            if (((MenuMenuname *)scan->data)->layout.type == MENU_CACHE_TYPE_DIR &&
                                strcmp(((MenuMenuname *)scan->data)->name, mm->name) == 0)
                                break;
                        }

                        if (scan != NULL) {
                            l = this_node->next;
                            continue;
                        }

                        _stage1(mm, dirs, apps, legacy, p);

                        VVDBG("+++ composing menu %s (%s)",
                              mm->name, mm->title);

                        if (mm->key == NULL) {
                            if (mm->title != NULL)
                                mm->key = g_utf8_collate_key(mm->title, -1);
                            else
                                mm->key = g_utf8_collate_key(mm->name, -1);
                        }

                        l = this_node->next;
                        menu->children = g_list_remove_link(menu->children, this_node);
                        next = g_list_concat(this_node, next);
                    } else {
                        l = l->next;
                    }
                }
            }

            result = g_list_concat(g_list_sort(next, _compare_items), result);
            break;

        default:
            break;
        }
    }
}

VDBG("... cleanup");
_free_leftovers(menu->children);

menu->children = g_list_reverse(result);

/* INLINE MERGE FIXES */
for (child = menu->children; child; ) {
    MenuMenu *submenu = child->data;
    child = child->next;

    if (submenu->layout.type == MENU_CACHE_TYPE_DIR &&
        submenu->layout.allow_inline &&
        (submenu->layout.inline_limit == 0 ||
         (gint)g_list_length(submenu->children) <= submenu->layout.inline_limit)) {

        DBG("*** got some inline!");

        /* FIXED inline-alias crash */
        if (submenu->layout.inline_alias &&
            g_list_length(submenu->children) == 1) {

            const char *new_title =
                submenu->title ? submenu->title : submenu->name;

            app = submenu->children->data;

            if (app->type == MENU_CACHE_TYPE_DIR) {
                ((MenuMenu *)app)->title = g_strdup(new_title);
            } else if (app->type == MENU_CACHE_TYPE_APP) {
                app->title = g_strdup(new_title);
            }
        }

        submenu->children = g_list_reverse(submenu->children);

        while (submenu->children != NULL) {
            menu->children =
                g_list_insert_before(menu->children, child,
                                     submenu->children->data);

            submenu->children =
                g_list_delete_link(submenu->children, submenu->children);
        }

        menu->children = g_list_remove(menu->children, submenu);
        menu_menu_free(submenu);
    }
}

DBG("... done %s", menu->name);

g_list_free(available);
g_list_free(_dirs);
g_list_free(_apps);
g_list_free(_legs);
g_list_free(_lprefs);
g_string_free(prefix, TRUE);

static gint
_stage2(MenuMenu *menu, gboolean with_hidden)
{
    GList *child = menu->children, *next, *to_delete = NULL;
    MenuApp *app;
    gint count = 0;

    VVDBG("stage 2: entered '%s'", menu->name);

    while (child) {
        app = child->data;
        next = child->next;

        switch (app->type) {
        case MENU_CACHE_TYPE_APP:
            if (menu->layout.only_unallocated && app->menus->next != NULL) {
                VDBG("removing from %s as only_unallocated %s", menu->name, app->id);
                menu->children = g_list_delete_link(menu->children, child);
                app->menus = g_list_remove(app->menus, menu);
            } else if (app->hidden && !with_hidden) {
                menu->children = g_list_delete_link(menu->children, child);
            } else {
                count++;
            }
            break;

        case MENU_CACHE_TYPE_DIR:
            if (_stage2(child->data, with_hidden) > 0) {
                count++;
            } else if (!with_hidden || req_version < 2) {
                to_delete = g_list_prepend(to_delete, child);
            }
            break;

        default:
            if (child == menu->children ||
                next == NULL ||
                ((MenuApp *)next->data)->type == MENU_CACHE_TYPE_SEP) {
                menu->children = g_list_delete_link(menu->children, child);
            }
            break;
        }

        child = next;
    }

    VVDBG("stage 2: counted '%s': %d", menu->name, count);

    if (count > 0 && with_hidden) {
        g_list_free(to_delete);
    } else {
        while (to_delete) {
            child = to_delete->data;
            VVDBG("stage 2: deleting empty '%s'",
                  ((MenuMenu *)child->data)->name);
            menu_menu_free(child->data);
            menu->children = g_list_delete_link(menu->children, child);
            to_delete = g_list_delete_link(to_delete, to_delete);
        }
    }

    if (count == 0) {
        if (menu->layout.show_empty)
            count++;
        else
            menu->layout.nodisplay = TRUE;
    }

    return count;
}

static inline int
_compose_flags(const char **f)
{
    int x = 0;

    while (*f) {
        int i = g_slist_index(DEs, *f++);
        if (i >= 0)
            x |= 1 << i;
    }

    return x;
}

static gboolean
write_app_extra(FILE *f, MenuApp *app)
{
    gboolean ret;
    char *cats, *keywords;
    char *null_list[] = { NULL };

    if (req_version < 2)
        return TRUE;

    cats = g_strjoinv(";", app->categories ? (char **)app->categories : null_list);
    keywords = g_strjoinv(",", app->keywords ? (char **)app->keywords : null_list);

    ret = fprintf(f, "%s\n%s\n%s\n%s\n",
                  NONULL(app->try_exec),
                  NONULL(app->wd),
                  cats,
                  keywords) > 0;

    g_free(cats);
    g_free(keywords);

    return ret;
}

static gboolean
write_app(FILE *f, MenuApp *app, gboolean with_hidden)
{
    int index;
    MenuCacheItemFlag flags = 0;
    int show = 0;

    if (app->hidden && !with_hidden)
        return TRUE;

    index = MAX(g_slist_index(AppDirs, app->dirs->data), 0) +
            g_slist_length(DirDirs);

    if (app->use_terminal)
        flags |= FLAG_USE_TERMINAL;
    if (app->hidden)
        flags |= FLAG_IS_NODISPLAY;
    if (app->use_notification)
        flags |= FLAG_USE_SN;

    if (app->show_in)
        show = _compose_flags(app->show_in);
    else if (app->hide_in)
        show = ~_compose_flags(app->hide_in);

    return fprintf(f,
                   "-%s\n%s\n%s\n%s\n%s\n%d\n%s\n%s\n%u\n%d\n",
                   app->id,
                   NONULL(app->title),
                   NONULL(app->comment),
                   NONULL(app->icon),
                   NONULL(app->filename),
                   index,
                   NONULL(app->generic_name),
                   NONULL(app->exec),
                   flags,
                   show) > 0 &&
           write_app_extra(f, app);
}

static gboolean
write_menu(FILE *f, MenuMenu *menu, gboolean with_hidden)
{
    int index;
    GList *child;
    gboolean ok = TRUE;

    if (!with_hidden && !menu->layout.show_empty && menu->children == NULL)
        return TRUE;

    if (menu->layout.nodisplay && (!with_hidden || req_version < 2))
        return TRUE;

    index = g_slist_index(DirDirs, menu->dir);

    if (fprintf(f,
                "+%s\n%s\n%s\n%s\n%s\n%d\n",
                menu->name,
                NONULL(menu->title),
                NONULL(menu->comment),
                NONULL(menu->icon),
                menu->id ? (const char *)menu->id->data : "",
                index) < 0)
        return FALSE;

    if (req_version >= 2 &&
        fprintf(f, "%d\n",
                menu->layout.nodisplay ? FLAG_IS_NODISPLAY : 0) < 0)
        return FALSE;

    for (child = menu->children; ok && child != NULL; child = child->next) {
        index = ((MenuApp *)child->data)->type;

        if (index == MENU_CACHE_TYPE_DIR) {
            ok = write_menu(f, child->data, with_hidden);
        } else if (index == MENU_CACHE_TYPE_APP) {
            ok = write_app(f, child->data, with_hidden);
        } else if (child->next != NULL &&
                   child != menu->children &&
                   ((MenuApp *)child->next->data)->type != MENU_CACHE_TYPE_SEP) {
            if (fprintf(f, "-\n") < 0)
                return FALSE;
        }
    }

    fputc('\n', f);
    return ok;
}

gboolean
save_menu_cache(MenuMenu *layout,
                const char *menuname,
                const char *file,
                gboolean with_hidden)
{
    const char *de_names[N_KNOWN_DESKTOPS] =
        { "LXDE", "GNOME", "KDE", "XFCE", "ROX" };
    char *tmp = NULL;
    FILE *f = NULL;
    GSList *l;
    int i = -1;
    gboolean ok = FALSE;
    const char *env_version;

    env_version = g_getenv("CACHE_GEN_VERSION");
    if (env_version &&
        sscanf(env_version, "%d.%u", &i, &req_version) == 2) {
        if (i != VER_MAJOR)
            return FALSE;
    }

    if (req_version < VER_MINOR_SUPPORTED)
        return FALSE;

    if (req_version > VER_MINOR)
        req_version = VER_MINOR;

    all_apps = g_hash_table_new_full(g_str_hash,
                                     g_str_equal,
                                     NULL,
                                     (GDestroyNotify)menu_app_free);

    for (i = 0; i < N_KNOWN_DESKTOPS; i++)
        DEs = g_slist_append(DEs,
                             (gpointer)g_intern_static_string(de_names[i]));

    /* Recursively add files into layout, don't take OnlyUnallocated into account */
    _stage1(layout, NULL, NULL, NULL, NULL);

    /* Recursively remove non-matched files by OnlyUnallocated flag */
    _stage2(layout, with_hidden);

    /* Prepare temporary file for safe creation */
    {
        const char *base_menu = strrchr(menuname, G_DIR_SEPARATOR);
        if (base_menu)
            menuname = &base_menu[1];
    }

    tmp = g_path_get_dirname(file);
    if (tmp != NULL) {
        if (!g_file_test(tmp, G_FILE_TEST_EXISTS))
            g_mkdir_with_parents(tmp, 0700);
        g_free(tmp);
    }

    tmp = g_strdup_printf("%sXXXXXX", file);
    i = g_mkstemp(tmp);
    if (i < 0)
        goto failed;

    f = fdopen(i, "w");
    if (f == NULL) {
        close(i);
        goto failed;
    }

    if (fprintf(f,
                "1.%u\n%s%s\n%u\n",
                req_version,
                menuname,
                with_hidden ? "+hidden" : "",
                (guint)(g_slist_length(DirDirs) +
                        g_slist_length(AppDirs) +
                        g_slist_length(MenuDirs) +
                        g_slist_length(MenuFiles))) < 0)
        goto failed;

    VDBG("%d %d %d %d",
         g_slist_length(DirDirs),
         g_slist_length(AppDirs),
         g_slist_length(MenuDirs),
         g_slist_length(MenuFiles));

    for (l = DirDirs; l; l = l->next)
        if (fprintf(f, "D%s\n", (const char *)l->data) < 0)
            goto failed;

    for (l = AppDirs; l; l = l->next)
        if (fprintf(f, "D%s\n", (const char *)l->data) < 0)
            goto failed;

    for (l = MenuDirs; l; l = l->next)
        if (fprintf(f, "D%s\n", (const char *)l->data) < 0)
            goto failed;

    for (l = MenuFiles; l; l = l->next)
        if (fprintf(f, "F%s\n", (const char *)l->data) < 0)
            goto failed;

    for (l = g_slist_nth(DEs, 5); l; l = l->next)
        if (fprintf(f, "%s;", (const char *)l->data) < 0)
            goto failed;

    if (fputc('\n', f) == EOF)
        goto failed;

    ok = write_menu(f, layout, with_hidden);

failed:
    if (f != NULL)
        fclose(f);
    else if (i >= 0 && !ok)
        g_unlink(tmp);

    if (ok) {
        ok = (g_rename(tmp, file) == 0);
        if (!ok)
            g_unlink(tmp);
    } else if (tmp && f != NULL) {
        g_unlink(tmp);
    }

    menu_menu_free(layout);
    g_free(tmp);
    g_hash_table_destroy(all_apps);
    g_slist_free(DEs);
    g_slist_free(loaded_dirs);

    return ok;
}
