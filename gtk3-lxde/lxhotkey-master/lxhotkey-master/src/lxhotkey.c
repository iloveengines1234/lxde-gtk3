/*
 * Copyright (C) 2016 Andriy Grytsenko <andrej@rep.kiev.ua>
 *               2023 Ingo Brückl
 *
 * This file is a part of LXHotkey project.
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
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "lxhotkey.h"
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_LIBUNISTRING
# include <unistdio.h>
# define ulc_printf(...) ulc_fprintf(stdout,__VA_ARGS__)
#else
# define ulc_printf printf
#endif

/* Standard error tracking context structures */
static GQuark LXKEYS_ERROR;
typedef enum {
    LXKEYS_BAD_ARGS,      /* invalid commandline arguments */
    LXKEYS_NOT_SUPPORTED, /* operation not supported */
    LXKEYS_RUNTIME_ERROR  /* system or backend communication error */
} LXHotkeyError;

/* Simple functions to manage LXHotkeyAttr data type */
static inline LXHotkeyAttr *lxhotkey_attr_new(void)
{
    return g_slice_new0(LXHotkeyAttr);
}

#define free_actions(acts) g_list_free_full(acts, (GDestroyNotify)lxkeys_attr_free)

static void lxkeys_attr_free(LXHotkeyAttr *data)
{
    g_free(data->name);
    g_list_free_full(data->values, g_free);
    free_actions(data->subopts);
    g_free(data->desc);
    g_slice_free(LXHotkeyAttr, data);
}

/* --- Environment Platform Detection Block --- */

static gchar *detect_active_window_manager(void)
{
    const char *xdg_current = g_getenv("XDG_CURRENT_DESKTOP");
    const char *wayland_disp = g_getenv("WAYLAND_DISPLAY");
    
    /* Wayland Environment Evaluation Traversal Path */
    if (wayland_disp != NULL) {
        if (xdg_current && g_ascii_strcasecmp(xdg_current, "LABWC") == 0) {
            return g_strdup("labwc");
        }
        return g_strdup("wayland-generic");
    }

    /* Fallback to standard desktop state engine strings */
    if (xdg_current && g_ascii_strcasecmp(xdg_current, "LXDE") == 0) {
        return g_strdup("Openbox"); /* Traditional LXDE standard engine mapping */
    }

    return g_strdup("unknown");
}

/* --- Modular Plugin Registry Lifecycles --- */

typedef struct LXHotkeyPlugin {
    struct LXHotkeyPlugin *next;
    gchar *name;
    LXHotkeyPluginInit *t;
} LXHotkeyPlugin;

static LXHotkeyPlugin *plugins = NULL;

FM_MODULE_DEFINE_TYPE(lxhotkey, LXHotkeyPluginInit, 1)
static gboolean fm_module_callback_lxhotkey(const char *name, gpointer init, int ver)
{
    LXHotkeyPlugin *plugin = g_new(LXHotkeyPlugin, 1);
    plugin->next = plugins;
    plugin->name = g_strdup(name);
    plugin->t = init;
    plugins = plugin;
    return TRUE;
}

typedef struct LXHotkeyGUIPlugin {
    struct LXHotkeyGUIPlugin *next;
    gchar *name;
    LXHotkeyGUIPluginInit *t;
} LXHotkeyGUIPlugin;

static LXHotkeyGUIPlugin *gui_plugins = NULL;

FM_MODULE_DEFINE_TYPE(lxhotkey_gui, LXHotkeyGUIPluginInit, 1)
static gboolean fm_module_callback_lxhotkey_gui(const char *name, gpointer init, int ver)
{
    LXHotkeyGUIPlugin *plugin = g_new(LXHotkeyGUIPlugin, 1);
    plugin->next = gui_plugins;
    plugin->name = g_strdup(name);
    plugin->t = init;
    gui_plugins = plugin;
    return TRUE;
}

static int _print_help(const char *cmd)
{
    printf(_("Usage: %s global [<key>]         - show keys bound to action(s)\n"), cmd);
    printf(_("       %s global <action> <key>  - bind a key to the action\n"), cmd);
    printf(_("       %s app [<key>]            - show keys bound to exec line\n"), cmd);
    printf(_("       %s app <exec> <key>       - bind a key to some exec line\n"), cmd);
    printf(_("       %s app <exec> --          - unbind all keys from exec line\n"), cmd);
    printf(_("       %s --gui=<type>           - start with GUI\n"), cmd);
    return 0;
}

/* Convert text line to LXHotkeyAttr list structure */
static GList *actions_from_str(const char *line, GError **error)
{
    GString *str = g_string_sized_new(16);
    LXHotkeyAttr *data = NULL, *attr = NULL;
    GList *list;

    data = lxhotkey_attr_new();
    list = g_list_prepend(NULL, data);
    for (; *line; line++) {
        switch (*line) {
        case '=':
            if (!data->name) {
                if (str->len == 0) goto _empty_name;
                data->name = g_strdup(str->str);
                g_string_truncate(str, 0);
            } else if (attr && !attr->name) {
                if (str->len == 0) goto _empty_opt;
                attr->name = g_strdup(str->str);
                g_string_truncate(str, 0);
            } else {
                g_string_append_c(str, *line);
            }
            break;
        case ':':
            if (!data->name) {
                if (str->len == 0) goto _empty_name;
                data->name = g_strdup(str->str);
            } else if (attr) {
                if (!attr->name) {
                    if (str->len == 0) goto _empty_opt;
                    attr->name = g_strdup(str->str);
                } else {
                    attr->values = g_list_prepend(NULL, g_strdup(str->str));
                }
            } else {
                data->values = g_list_prepend(NULL, g_strdup(str->str));
            }
            g_string_truncate(str, 0);
            attr = lxhotkey_attr_new();
            data->subopts = g_list_prepend(data->subopts, attr);
            break;
        case '&':
            if (!data->name) {
                if (str->len == 0) goto _empty_name;
                data->name = g_strdup(str->str);
            } else if (attr) {
                if (!attr->name) {
                    if (str->len == 0) goto _empty_opt;
                    attr->name = g_strdup(str->str);
                } else {
                    attr->values = g_list_prepend(NULL, g_strdup(str->str));
                }
            } else {
                data->values = g_list_prepend(NULL, g_strdup(str->str));
            }
            g_string_truncate(str, 0);
            attr = NULL;
            data->subopts = g_list_reverse(data->subopts);
            data = lxhotkey_attr_new();
            list = g_list_prepend(list, data);
            break;
        case '\\':
            break;
        default:
            g_string_append_c(str, *line);
        }
    }
    if (!data->name) {
        if (str->len == 0) goto _empty_name;
        data->name = g_strdup(str->str);
    } else if (attr) {
        if (!attr->name) {
            if (str->len == 0) goto _empty_opt;
            attr->name = g_strdup(str->str);
        } else {
            attr->values = g_list_prepend(NULL, g_strdup(str->str));
        }
    } else {
        data->values = g_list_prepend(NULL, g_strdup(str->str));
    }
    list = g_list_reverse(list);
    g_string_free(str, TRUE);
    return list;

_empty_opt:
    g_set_error_literal(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS, _("empty option name."));
    goto _failed;
_empty_name:
    g_set_error_literal(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS, _("empty action name."));
_failed:
    free_actions(list);
    g_string_free(str, TRUE);
    return NULL;
}

static gboolean validate_actions(const GList *act, const GList *origin,
                                 const LXHotkeyAttr *action, gchar *wm_name,
                                 GError **error)
{
    const LXHotkeyAttr *data, *ordata;
    const GList *l, *olist;
    char *endptr;

    if (!origin)
        return FALSE;
    if (action)
        olist = action->subopts;
    else
        olist = origin;
    while (act) {
        data = act->data;
        for (l = olist; l; l = l->next) {
            if (g_strcmp0(data->name, (ordata = l->data)->name) == 0)
                break;
        }
        if (l == NULL) {
            if (action)
                g_set_error(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS,
                            _("no matching option '%s' found for action '%s'."),
                            data->name, action->name);
            else
                g_set_error(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS,
                            _("action '%s' isn't supported by WM %s."),
                            data->name, wm_name);
            return FALSE;
        }
        if (data->values != NULL && ordata->values != NULL) {
            for (l = ordata->values; l; l = l->next) {
                if (g_strcmp0(data->values->data, l->data) == 0 ||
                    (strtol(data->values->data, &endptr, 10) < LONG_MAX &&
                     ((g_strcmp0(l->data, "#") == 0 && endptr[0] == '\0') ||
                      (g_strcmp0(l->data, "%") == 0 && endptr[0] == '%'))))
                    break;
            }
            if (l == NULL) {
                if (action)
                    g_set_error(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS,
                                _("value '%s' is not supported for option '%s'."),
                                (char *)data->values->data, data->name);
                else
                    g_set_error(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS,
                                _("value '%s' is not supported for action '%s'."),
                                (char *)data->values->data, data->name);
                return FALSE;
            }
        }
        if (!data->subopts) ;
        else if (ordata->has_actions) {
            if (!validate_actions(data->subopts, origin, NULL, wm_name, error))
                return FALSE;
        } else if (!ordata->subopts) {
            g_set_error(error, LXKEYS_ERROR, LXKEYS_BAD_ARGS,
                        _("action '%s' does not support options."), data->name);
            return FALSE;
        } else if (!validate_actions(data->subopts, origin, ordata, wm_name, error))
            return FALSE;
        act = act->next;
    }
    return TRUE;
}

static void print_suboptions(GList *sub, int indent)
{
    indent += 3;
    while (sub) {
        LXHotkeyAttr *action = sub->data;
        if (action->values && action->values->data)
            printf("%*s%s=%s\n", indent, "", action->name, (char *)action->values->data);
        else
            printf("%*s%s\n", indent, "", action->name);
        print_suboptions(action->subopts, indent);
        sub = sub->next;
    }
}

/* --- Core Application Entrance Execution Point --- */

int main(int argc, char *argv[])
{
    const char *cmd;
    gchar *wm_name = NULL;
    LXHotkeyPlugin *plugin = NULL;
    LXHotkeyGUIPlugin *gui_plugin = NULL;
    int ret = 1;
    gpointer config = NULL;
    GError *error = NULL;
    gboolean do_gui = FALSE;

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    setlocale(LC_MESSAGES, "");
#endif

    /* Safer argument parsing constraints logic */
    if (argc < 2) {
        cmd = "gui=gtk";
    } else {
        cmd = argv[1];
        if (cmd[0] == '-' && cmd[1] == '-') {
            cmd += 2;
        } else if (cmd[0] == '-') {
            cmd += 1;
        }
    }

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0) {
        return _print_help(argv[0]);
    }

    if (strncmp(cmd, "gui=", 4) == 0) {
        do_gui = TRUE;
        cmd += 4;
    } else if (strcmp(cmd, "gui") == 0) {
        do_gui = TRUE;
        cmd = "gtk";
    }

    /* Initialize runtime module architecture paths via LibFM backend */
    fm_init(NULL);
    fm_modules_add_directory(PACKAGE_PLUGINS_DIR);
    fm_module_register_lxhotkey();
    if (do_gui)
        fm_module_register_lxhotkey_gui();

    LXKEYS_ERROR = g_quark_from_static_string("lxhotkey-error");

    CHECK_MODULES();
    if (do_gui) {
        for (gui_plugin = gui_plugins; gui_plugin; gui_plugin = gui_plugin->next) {
            if (g_ascii_strcasecmp(gui_plugin->name, cmd) == 0)
                break;
        }
    }

    if (gui_plugin && gui_plugin->t->init)
        gui_plugin->t->init(argc, argv);

    /* MODERN FIX: Modular runtime discovery abstraction */
    wm_name = detect_active_window_manager();
    if (!wm_name || strcmp(wm_name, "unknown") == 0) {
        g_set_error_literal(&error, LXKEYS_ERROR, LXKEYS_NOT_SUPPORTED,
                            _("Could not determine active window manager environment context."));
        goto _exit;
    }

    for (plugin = plugins; plugin; plugin = plugin->next) {
        if (g_ascii_strcasecmp(plugin->name, wm_name) == 0)
            break;
    }

    if (!plugin) {
        g_set_error(&error, LXKEYS_ERROR, LXKEYS_NOT_SUPPORTED,
                    _("Could not find a plugin for window manager %s."), wm_name);
        goto _exit;
    }

    config = plugin->t->load(NULL, &error);
    if (!config) {
        g_prefix_error(&error, _("Problems loading configuration: "));
        goto _exit;
    }

    if (do_gui) {
        if (gui_plugin && gui_plugin->t->run)
            gui_plugin->t->run(wm_name, plugin->t, &config, &error);
        else
            g_set_error(&error, LXKEYS_ERROR, LXKEYS_NOT_SUPPORTED,
                        _("GUI type %s currently isn't supported."), cmd);
        goto _exit;
    }

    /* Commandline mode handling routing paths */
    if (strcmp(argv[1], "global") == 0) {
        if (argc > 3) {
            LXHotkeyGlobal data;

            if (plugin->t->set_wm_key == NULL)
                goto _not_supported;
            data.actions = actions_from_str(argv[2], &error);
            if (error ||
                (plugin->t->get_wm_actions != NULL &&
                 !validate_actions(data.actions, plugin->t->get_wm_actions(config, &error),
                                   NULL, wm_name, &error))) {
                g_prefix_error(&error, _("Invalid request: "));
                goto _exit;
            }
            data.accel1 = argv[3];
            data.accel2 = (argc > 4) ? argv[4] : NULL;

            if (!plugin->t->set_wm_key(config, &data, &error) ||
                !plugin->t->save(config, &error)) {
                g_prefix_error(&error, _("Problems saving configuration: "));
                free_actions(data.actions);
                goto _exit;
            }
            free_actions(data.actions);
        } else {
            const char *mask = (argc > 2) ? argv[2] : NULL;
            GList *keys, *key;
            LXHotkeyGlobal *data;
            GList *act;
            LXHotkeyAttr *action;

            if (plugin->t->get_wm_keys == NULL)
                goto _not_supported;
            keys = plugin->t->get_wm_keys(config, mask, NULL);
            ulc_printf(" %-24s %s\n", _("ACTION(s)"), _("KEY(s)"));
            for (key = keys; key; key = key->next) {
                data = key->data;
                for (act = data->actions; act; act = act->next) {
                    action = act->data;
                    if (act != data->actions)
                        printf("+ %s\n", action->name);
                    else if (data->accel2)
                        ulc_printf("%-24s %s %s\n", action->name, data->accel1, data->accel2);
                    else
                        ulc_printf("%-24s %s\n", action->name, data->accel1);
                    print_suboptions(action->subopts, 0);
                }
            }
            g_list_free(keys);
        }
    } else if (strcmp(argv[1], "app") == 0) {
        if (argc > 3) {
            GList *keys = NULL;
            LXHotkeyApp data;

            if (plugin->t->set_app_key == NULL)
                goto _not_supported;
            if (plugin->t->get_app_keys != NULL)
                keys = plugin->t->get_app_keys(config, argv[2], NULL);
            if (keys && keys->next)
                goto _not_supported;
            data.accel2 = NULL;
            if (strcmp(argv[3], "--") == 0) {
                data.accel1 = NULL;
            } else if (keys && ((LXHotkeyApp *)keys->data)->accel1) {
                data.accel1 = ((LXHotkeyApp *)keys->data)->accel1;
                data.accel2 = argv[3];
            } else {
                data.accel1 = argv[3];
            }
            g_list_free(keys);
            cmd = strchr(argv[2], '&');
            if (cmd) {
                data.options = actions_from_str(&cmd[1], &error);
                if (error ||
                    (plugin->t->get_app_options != NULL &&
                     !validate_actions(data.options, plugin->t->get_app_options(config, &error),
                                       NULL, wm_name, &error))) {
                    g_prefix_error(&error, _("Invalid request: "));
                    free_actions(data.options);
                    goto _exit;
                }
                data.exec = g_strndup(argv[2], cmd - argv[2]);
            } else {
                data.options = NULL;
                data.exec = g_strdup(argv[2]);
            }
            if (!plugin->t->set_app_key(config, &data, &error) ||
                !plugin->t->save(config, &error)) {
                g_prefix_error(&error, _("Problems saving configuration: "));
                free_actions(data.options);
                g_free(data.exec);
                goto _exit;
            }
            free_actions(data.options);
            g_free(data.exec);
        } else {
            const char *mask = (argc > 2) ? argv[2] : NULL;
            GList *keys, *key;
            LXHotkeyApp *data;

            if (plugin->t->get_app_keys == NULL)
                goto _not_supported;
            keys = plugin->t->get_app_keys(config, mask, NULL);
            ulc_printf(" %-48s %s\n", _("EXEC"), _("KEY(s)"));
            for (key = keys; key; key = key->next) {
                data = key->data;
                if (data->accel2)
                    ulc_printf("%-48s %s %s\n", data->exec, data->accel1, data->accel2);
                else
                    ulc_printf("%-48s %s\n", data->exec, data->accel1);
                print_suboptions(data->options, 0);
            }
            g_list_free(keys);
        }
    } else {
        goto _exit;
    }
    ret = 0;
    goto _exit;

_not_supported:
    g_set_error_literal(&error, LXKEYS_ERROR, LXKEYS_NOT_SUPPORTED,
                        _("Requested operation isn't supported."));

_exit:
    if (config)
        plugin->t->free(config);
    if (error) {
        if (gui_plugin && gui_plugin->t->alert)
            gui_plugin->t->alert(error);
        else
            fprintf(stderr, "LXHotkey: %s\n", error->message);
        g_error_free(error);
    }
    fm_module_unregister_type("lxhotkey");
    if (do_gui)
        fm_module_unregister_type("lxhotkey_gui");
    while (plugins) {
        plugin = plugins;
        plugins = plugin->next;
        g_free(plugin->name);
        g_free(plugin);
    }
    while (gui_plugins) {
        gui_plugin = gui_plugins;
        gui_plugins = gui_plugin->next;
        g_free(gui_plugin->name);
        g_free(gui_plugin);
    }
    g_free(wm_name);

    return ret;
}