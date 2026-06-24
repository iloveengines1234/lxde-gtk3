/*
 * Copyright (C) 2016-2021 Andriy Grytsenko <andrej@rep.kiev.ua>
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
# include "config.h"
#endif

#define WANT_OPTIONS_EQUAL

#include "lxhotkey.h"
#include "edit.h"

#include <stdlib.h>
#include <string.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#if !GTK_CHECK_VERSION(2, 21, 0)
# define GDK_KEY_Tab            GDK_Tab
# define GDK_KEY_BackSpace      GDK_BackSpace
# define GDK_KEY_Escape         GDK_Escape
# define GDK_KEY_space          GDK_space
#endif

enum {
    EDIT_MODE_NEW,
    EDIT_MODE_EDIT
};

static void on_value_changed(GtkComboBox *box, PluginData *data);

static GList *copy_options(const GList *options)
{
    GList *new_opts = NULL;
    const GList *l;

    for (l = options; l; l = l->next)
    {
        LXHotkeyAttr *attr = l->data;
        LXHotkeyAttr *new_attr = g_new0(LXHotkeyAttr, 1);
        GList *v;

        new_attr->name = g_strdup(attr->name);
        for (v = attr->values; v; v = v->next)
            new_attr->values = g_list_append(new_attr->values, g_strdup(v->data));
        if (attr->subopts)
            new_attr->subopts = copy_options(attr->subopts);

        new_opts = g_list_append(new_opts, new_attr);
    }
    return new_opts;
}

void _edit_cleanup(PluginData *data)
{
    if (data->edit_window == NULL)
        return;

    gtk_widget_destroy(GTK_WIDGET(data->edit_window));
    data->edit_window = NULL;

    lxhotkey_options_free(data->edit_options_copy);
    data->edit_options_copy = NULL;
    data->edit_template = NULL;
}

static void fill_options_tree(GtkTreeView *tree, const GList *options)
{
    GtkTreeStore *model = GTK_TREE_STORE(gtk_tree_view_get_model(tree));
    const GList *l;

    gtk_tree_store_clear(model);

    for (l = options; l; l = l->next)
    {
        LXHotkeyAttr *attr = l->data;
        GtkTreeIter iter;
        GString *val = g_string_new(NULL);
        GList *v;

        for (v = attr->values; v; v = v->next)
        {
            if (val->len > 0)
                g_string_append(val, ", ");
            g_string_append(val, _((char *)v->data));
        }

        gtk_tree_store_append(model, &iter, NULL);
        gtk_tree_store_set(model, &iter, 0, _(attr->name), 1, val->str, 2, attr, -1);
        g_string_free(val, TRUE);
    }
}

static gboolean on_key_event(GtkButton *test, GdkEventKey *event, PluginData *data)
{
    GdkModifierType state;
    char *text;
    const char *label1;
    const char *label2;

    if (event->keyval == GDK_KEY_Tab)
        return FALSE;

    gdk_window_get_pointer(gtk_widget_get_window(GTK_WIDGET(test)), NULL, NULL, &state);

    if ((state & GDK_SUPER_MASK) == 0 && (state & GDK_MOD4_MASK) != 0)
        state |= GDK_SUPER_MASK;

    state &= gtk_accelerator_get_default_mod_mask();

    if (event->is_modifier)
    {
        if (state != 0)
        {
            text = gtk_accelerator_get_label(0, state);
            gtk_button_set_label(test, text);
            g_free(text);
        }
        else
        {
            gtk_button_set_label(test, g_object_get_data(G_OBJECT(test), "original_label"));
        }
        return FALSE;
    }

    if (event->type != GDK_KEY_PRESS)
        return FALSE;

    if (state == 0 && event->keyval == GDK_KEY_Escape)
    {
        gtk_button_set_label(test, g_object_get_data(G_OBJECT(test), "original_label"));
        goto _done;
    }

    if (state == 0 && event->keyval == GDK_KEY_BackSpace)
    {
        gtk_button_set_label(test, "");
        g_object_set_data(G_OBJECT(test), "accelerator_name", NULL);
        g_object_set_data(G_OBJECT(test), "original_label", NULL);

_done:
        label1 = gtk_button_get_label(GTK_BUTTON(data->edit_key1));
        label2 = gtk_button_get_label(GTK_BUTTON(data->edit_key2));
        gtk_action_set_sensitive(data->edit_apply_button,
                                 (label1 && label1[0] != '\0') || (label2 && label2[0] != '\0'));

        if (data->edit_exec)
            gtk_widget_grab_focus(GTK_WIDGET(data->edit_exec));
        else
            gtk_widget_grab_focus(GTK_WIDGET(data->edit_tree));
        return FALSE;
    }

    text = gtk_accelerator_get_label(event->keyval, state);
    gtk_button_set_label(test, text);

    if (event->length != 0 && (state == 0 || state == GDK_SHIFT_MASK ||
                               state == GDK_CONTROL_MASK || state == GDK_MOD1_MASK) &&
        (event->keyval != GDK_KEY_space || state != GDK_MOD1_MASK))
    {
        GtkWidget* dlg;
        dlg = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                     _("Key combination '%s' cannot be used as a global hotkey, sorry."), text);
        g_free(text);
        gtk_window_set_title(GTK_WINDOW(dlg), _("Error"));
        gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        gtk_button_set_label(test, g_object_get_data(G_OBJECT(test), "original_label"));
        
        label1 = gtk_button_get_label(GTK_BUTTON(data->edit_key1));
        label2 = gtk_button_get_label(GTK_BUTTON(data->edit_key2));
        gtk_action_set_sensitive(data->edit_apply_button,
                                 (label1 && label1[0] != '\0') || (label2 && label2[0] != '\0'));
        return FALSE;
    }

    g_object_set_data_full(G_OBJECT(test), "original_label", text, g_free);
    text = gtk_accelerator_name(event->keyval, state);

    if (data->use_primary)
    {
        GString *gstr = g_string_new(text);
        char *match = strstr(gstr->str, "<Primary>");
        if (match != NULL)
        {
            gsize offset = match - gstr->str;
            g_string_erase(gstr, offset, 9);
            g_string_insert(gstr, offset, "<Control>");
        }
        g_free(text);
        text = g_string_free(gstr, FALSE);
    }

    g_object_set_data_full(G_OBJECT(test), "accelerator_name", text, g_free);
    gtk_action_set_sensitive(data->edit_apply_button, TRUE);

    if (data->edit_exec)
        gtk_widget_grab_focus(GTK_WIDGET(data->edit_exec));
    else
        gtk_widget_grab_focus(GTK_WIDGET(data->edit_tree));
    return FALSE;
}

static gboolean on_key_grab(GtkWidget *widget, GdkEventButton *event, PluginData *data)
{
    if (event->type == GDK_BUTTON_PRESS && event->button == 1)
    {
        GdkGrabStatus status;
        status = gdk_keyboard_grab(gtk_widget_get_window(widget), FALSE, event->time);
        if (status == GDK_GRAB_SUCCESS)
        {
            g_signal_connect(widget, "key-press-event", G_CALLBACK(on_key_event), data);
            g_signal_connect(widget, "key-release-event", G_CALLBACK(on_key_event), data);
            gtk_button_set_label(GTK_BUTTON(widget), _("Press keys..."));
        }
    }
    return FALSE;
}

static gboolean on_key_ungrab(GtkWidget *widget, GdkEventFocus *event, PluginData *data)
{
    gdk_keyboard_ungrab(GDK_CURRENT_TIME);
    g_signal_handlers_disconnect_by_func(widget, G_CALLBACK(on_key_event), data);
    gtk_button_set_label(GTK_BUTTON(widget), g_object_get_data(G_OBJECT(widget), "original_label"));
    return FALSE;
}

static GtkWidget *key_button_new(PluginData *data, const char *accel)
{
    GtkWidget *button = gtk_button_new();
    char *text = NULL;

    if (accel && accel[0] != '\0')
    {
        guint key;
        GdkModifierType mods;
        gtk_accelerator_parse(accel, &key, &mods);
        text = gtk_accelerator_get_label(key, mods);
        g_object_set_data_full(G_OBJECT(button), "accelerator_name", g_strdup(accel), g_free);
    }

    g_object_set_data_full(G_OBJECT(button), "original_label", g_strdup(text ? text : ""), g_free);
    gtk_button_set_label(GTK_BUTTON(button), text ? text : "");
    g_free(text);

    g_signal_connect(button, "button-press-event", G_CALLBACK(on_key_grab), data);
    g_signal_connect(button, "focus-out-event", G_CALLBACK(on_key_ungrab), data);

    return button;
}

static void on_cancel_button(GtkButton *button, PluginData *data)
{
    _edit_cleanup(data);
}

static void on_apply_button(GtkButton *button, PluginData *data)
{
    GError *error = NULL;
    const char *accel1;
    const char *accel2;
    gboolean success = FALSE;

    accel1 = g_object_get_data(G_OBJECT(data->edit_key1), "accelerator_name");
    accel2 = g_object_get_data(G_OBJECT(data->edit_key2), "accelerator_name");

    if (data->edit_exec)
    {
        LXHotkeyApp app;
        app.exec = (char *)gtk_entry_get_text(data->edit_exec);
        app.accel1 = (char *)accel1;
        app.accel2 = (char *)accel2;
        success = data->cb->set_app_key(*data->config, &app, &error);
    }
    else
    {
        LXHotkeyGlobal act;
        GtkTreeModel *model = gtk_combo_box_get_model(data->edit_actions);
        GtkTreeIter iter;
        LXHotkeyAttr *attr;

        if (!gtk_combo_box_get_active_iter(data->edit_actions, &iter))
            return;

        gtk_tree_model_get(model, &iter, 1, &attr, -1);
        act.actions = g_list_append(NULL, attr);
        act.accel1 = (char *)accel1;
        act.accel2 = (char *)accel2;
        success = data->cb->set_wm_key(*data->config, &act, &error);
        g_list_free(act.actions);
    }

    if (success)
    {
        gtk_action_set_sensitive(data->save_action, TRUE);
        _main_refresh(data);
        _edit_cleanup(data);
    }
    else
    {
        _show_error(_("Cannot save keybinding: "), error);
        g_error_free(error);
    }
}

static void on_exec_changed(GtkEditable *editable, PluginData *data)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(editable));
    const char *label1 = gtk_button_get_label(GTK_BUTTON(data->edit_key1));
    const char *label2 = gtk_button_get_label(GTK_BUTTON(data->edit_key2));

    gtk_action_set_sensitive(data->edit_apply_button,
                             text && text[0] != '\0' &&
                             ((label1 && label1[0] != '\0') || (label2 && label2[0] != '\0')));
}

static void on_action_changed(GtkComboBox *box, PluginData *data)
{
    GtkTreeModel *model = gtk_combo_box_get_model(box);
    GtkTreeIter iter;
    LXHotkeyAttr *attr;
    const char *label1 = gtk_button_get_label(GTK_BUTTON(data->edit_key1));
    const char *label2 = gtk_button_get_label(GTK_BUTTON(data->edit_key2));

    if (!gtk_combo_box_get_active_iter(box, &iter))
        return;

    gtk_tree_model_get(model, &iter, 1, &attr, -1);
    gtk_action_set_sensitive(data->edit_apply_button,
                             attr && ((label1 && label1[0] != '\0') || (label2 && label2[0] != '\0')));
}

static void on_value_changed(GtkComboBox *box, PluginData *data)
{
    LXHotkeyAttr *opt;
    const char *value;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gdouble num = 0;
    long div;

    model = gtk_combo_box_get_model(box);
    if (!gtk_combo_box_get_active_iter(box, &iter))
        goto _general;

    gtk_tree_model_get(model, &iter, 1, &value, -1);
    if (!value)
        goto _general;

    if (value[0] == '#')
    {
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(data->edit_value_num), -1000.0, +1000.0);
        if (data->edit_mode == EDIT_MODE_EDIT &&
            gtk_tree_selection_get_selected(gtk_tree_view_get_selection(data->edit_tree), &model, &iter))
        {
            gtk_tree_model_get(model, &iter, 2, &opt, -1);
            if (opt && opt->values && opt->values->data)
                num = strtol(opt->values->data, NULL, 10);
        }
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->edit_value_num), num);
        gtk_widget_show(data->edit_value_num);
        gtk_widget_hide(data->edit_value_num_label);
    }
    else if (value[0] == '%')
    {
        gtk_spin_button_set_range(GTK_SPIN_BUTTON(data->edit_value_num), 0.0, +100.0);
        if (data->edit_mode == EDIT_MODE_EDIT &&
            gtk_tree_selection_get_selected(gtk_tree_view_get_selection(data->edit_tree), &model, &iter))
        {
            gtk_tree_model_get(model, &iter, 2, &opt, -1);
            if (opt && opt->values && opt->values->data)
            {
                const char *curr_str = opt->values->data;
                char *next_token = NULL;
                num = strtol(curr_str, &next_token, 10);
                
                if (next_token && *next_token == '/' && *(next_token + 1) != '\0')
                {
                    div = strtol(next_token + 1, NULL, 10);
                    div = MAX(div, 1);
                    num = (num * 100) / div;
                }
            }
        }
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->edit_value_num), num);
        gtk_widget_show(data->edit_value_num);
        gtk_label_set_text(GTK_LABEL(data->edit_value_num_label), "%");
        gtk_widget_show(data->edit_value_num_label);
    }
    else
    {
_general:
        gtk_widget_hide(data->edit_value_num);
        gtk_widget_hide(data->edit_value_num_label);
    }
}

static void on_frame_apply(GtkButton *btn, PluginData *data)
{
    GtkTreeModel *model = gtk_combo_box_get_model(data->edit_values);
    GtkTreeIter iter;
    const char *val;
    LXHotkeyAttr *opt = NULL;

    if (data->edit_mode == EDIT_MODE_EDIT)
    {
        if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(data->edit_tree), &model, &iter))
            gtk_tree_model_get(model, &iter, 2, &opt, -1);
    }

    if (!opt)
        return;

    g_list_free_full(opt->values, g_free);
    opt->values = NULL;

    if (gtk_widget_get_visible(data->edit_value))
    {
        val = gtk_entry_get_text(data->edit_value);
        if (val && val[0] != '\0')
            opt->values = g_list_append(opt->values, g_strdup(val));
    }
    else if (gtk_widget_get_visible(data->edit_value_num))
    {
        char buf[32];
        g_snprintf(buf, sizeof(buf), "%d", (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->edit_value_num)));
        opt->values = g_list_append(opt->values, g_strdup(buf));
    }
    else if (gtk_combo_box_get_active_iter(data->edit_values, &iter))
    {
        gtk_tree_model_get(model, &iter, 1, &val, -1);
        if (val && val[0] != '\0')
            opt->values = g_list_append(opt->values, g_strdup(val));
    }

    fill_options_tree(data->edit_tree, data->edit_options_copy);
    gtk_widget_hide(data->edit_frame);
}

static void on_frame_cancel(GtkButton *btn, PluginData *data)
{
    gtk_widget_hide(data->edit_frame);
}

static void on_option_selected(GtkTreeSelection *sel, PluginData *data)
{
    gboolean has_selection = gtk_tree_selection_get_selected(sel, NULL, NULL);
    gtk_action_set_sensitive(data->rm_option_button, has_selection);
    gtk_action_set_sensitive(data->edit_option_button, has_selection);
}

static void on_add_option(GtkAction *act, PluginData *data)
{
    data->edit_mode = EDIT_MODE_NEW;
    gtk_widget_show(data->edit_frame);
}

static void on_edit_option(GtkAction *act, PluginData *data)
{
    GtkTreeModel *model;
    GtkTreeIter iter;
    LXHotkeyAttr *opt = NULL;

    if (!gtk_tree_selection_get_selected(gtk_tree_view_get_selection(data->edit_tree), &model, &iter))
        return;

    gtk_tree_model_get(model, &iter, 2, &opt, -1);
    if (!opt)
        return;

    data->edit_mode = EDIT_MODE_EDIT;
    gtk_label_set_text(GTK_LABEL(data->edit_option_name), _(opt->name));

    gtk_widget_hide(data->edit_value);
    gtk_widget_hide(GTK_WIDGET(data->edit_values));
    gtk_widget_hide(data->edit_value_num);
    gtk_widget_hide(data->edit_value_num_label);

    gtk_widget_show(data->edit_frame);
    on_value_changed(data->edit_values, data->edit_value);
}

void _edit_action(PluginData *data, GError **error)
{
    GtkUIManager *ui;
    GtkWidget *win, *vbox, *hbox, *table, *lbl, *scrwin;
    GtkActionGroup *grp;
    LXHotkeyGlobal *gl = NULL;
    LXHotkeyApp *app = NULL;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean is_action = (data->current_page == data->acts);
    const char *accel1 = NULL, *accel2 = NULL;

    if (data->edit_window)
        return;

    if (is_action)
    {
        if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(data->acts), &model, &iter))
            gtk_tree_model_get(model, &iter, 4, &gl, -1);
        data->edit_template = data->cb->get_wm_actions(*data->config, error);
    }
    else
    {
        if (gtk_tree_selection_get_selected(gtk_tree_view_get_selection(data->apps), &model, &iter))
            gtk_tree_model_get(model, &iter, 3, &app, -1);
        data->edit_template = data->cb->get_app_options(*data->config, error);
    }

    win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    data->edit_window = GTK_WINDOW(win);
    gtk_window_set_title(data->edit_window, is_action ? _("Edit Action") : _("Edit Program"));
    gtk_window_set_default_size(data->edit_window, 600, 400);
    g_signal_connect_swapped(win, "destroy", G_CALLBACK(_edit_cleanup), data);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(win), vbox);

    table = gtk_table_new(3, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 4);

    lbl = gtk_label_new(is_action ? _("Action:") : _("Command:"));
    gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 0, 1, GTK_FILL, 0, 4, 4);

    if (is_action)
    {
        GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
        GtkCellRenderer *render = gtk_cell_renderer_text_new();
        const GList *l;

        data->edit_actions = GTK_COMBO_BOX(gtk_combo_box_new_with_model(GTK_TREE_MODEL(store)));
        gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(data->edit_actions), render, TRUE);
        gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(data->edit_actions), render, "text", 0, NULL);

        for (l = data->edit_template; l; l = l->next)
        {
            LXHotkeyAttr *tmpl = l->data;
            gtk_list_store_append(store, &iter);
            gtk_list_store_set(store, &iter, 0, _(tmpl->name), 1, tmpl, -1);
            if (gl && gl->actions && strcmp(((LXHotkeyAttr*)gl->actions->data)->name, tmpl->name) == 0)
                gtk_combo_box_set_active_iter(data->edit_actions, &iter);
        }
        g_object_unref(store);
        gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(data->edit_actions), 1, 2, 0, 1);
        g_signal_connect(data->edit_actions, "changed", G_CALLBACK(on_action_changed), data);
    }
    else
    {
        data->edit_exec = GTK_ENTRY(gtk_entry_new());
        if (app)
            gtk_entry_set_text(data->edit_exec, app->exec);
        gtk_table_attach_defaults(GTK_TABLE(table), GTK_WIDGET(data->edit_exec), 1, 2, 0, 1);
        g_signal_connect(data->edit_exec, "changed", G_CALLBACK(on_exec_changed), data);
    }

    if (gl) { accel1 = gl->accel1; accel2 = gl->accel2; }
    if (app) { accel1 = app->accel1; accel2 = app->accel2; }

    lbl = gtk_label_new(_("Hotkey 1:"));
    gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 1, 2, GTK_FILL, 0, 4, 4);
    data->edit_key1 = key_button_new(data, accel1);
    gtk_table_attach_defaults(GTK_TABLE(table), data->edit_key1, 1, 2, 1, 2);

    lbl = gtk_label_new(_("Hotkey 2:"));
    gtk_table_attach(GTK_TABLE(table), lbl, 0, 1, 2, 3, GTK_FILL, 0, 4, 4);
    data->edit_key2 = key_button_new(data, accel2);
    gtk_table_attach_defaults(GTK_TABLE(table), data->edit_key2, 1, 2, 2, 3);

    ui = gtk_ui_manager_new();
    grp = gtk_action_group_new("EditActions");
    
    data->edit_apply_button = gtk_action_new("Apply", _("Apply"), NULL, GTK_STOCK_APPLY);
    data->add_option_button = gtk_action_new("AddOption", _("Add Option"), NULL, GTK_STOCK_ADD);
    data->rm_option_button = gtk_action_new("RemoveOption", _("Remove Option"), NULL, GTK_STOCK_REMOVE);
    data->edit_option_button = gtk_action_new("EditOption", _("Edit Option"), NULL, GTK_STOCK_EDIT);

    gtk_action_group_add_action(grp, data->edit_apply_button);
    gtk_action_group_add_action(grp, data->add_option_button);
    gtk_action_group_add_action(grp, data->rm_option_button);
    gtk_action_group_add_action(grp, data->edit_option_button);
    gtk_ui_manager_insert_action_group(ui, grp, 0);

    g_signal_connect(data->add_option_button, "activate", G_CALLBACK(on_add_option), data);
    g_signal_connect(data->edit_option_button, "activate", G_CALLBACK(on_edit_option), data);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    scrwin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrwin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(hbox), scrwin, TRUE, TRUE, 0);

    GtkTreeStore *ts = gtk_tree_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
    data->edit_tree = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(ts)));
    g_object_unref(ts);

    gtk_tree_view_insert_column_with_attributes(data->edit_tree, 0, _("Option"), gtk_cell_renderer_text_new(), "text", 0, NULL);
    gtk_tree_view_insert_column_with_attributes(data->edit_tree, 1, _("Value"), gtk_cell_renderer_text_new(), "text", 1, NULL);
    gtk_container_add(GTK_CONTAINER(scrwin), GTK_WIDGET(data->edit_tree));

    g_signal_connect(gtk_tree_view_get_selection(data->edit_tree), "changed", G_CALLBACK(on_option_selected), data);

    if (gl && gl->actions)
        data->edit_options_copy = copy_options(((LXHotkeyAttr*)gl->actions->data)->subopts);

    fill_options_tree(data->edit_tree, data->edit_options_copy);

    data->edit_frame = gtk_frame_new(_("Set Parameter"));
    gtk_box_pack_start(GTK_BOX(hbox), data->edit_frame, FALSE, FALSE, 4);

    GtkWidget *xbox = gtk_vbox_new(FALSE, 4);
    data->edit_option_name = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(xbox), data->edit_option_name, FALSE, FALSE, 2);

    data->edit_value = GTK_ENTRY(gtk_entry_new());
    gtk_box_pack_start(GTK_BOX(xbox), GTK_WIDGET(data->edit_value), FALSE, FALSE, 2);

    GtkListStore *vs = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    data->edit_values = GTK_COMBO_BOX(gtk_combo_box_new_with_model(GTK_TREE_MODEL(vs)));
    g_object_unref(vs);
    gtk_box_pack_start(GTK_BOX(xbox), GTK_WIDGET(data->edit_values), FALSE, FALSE, 2);

    data->edit_value_num = gtk_spin_button_new_with_range(0, 100, 1);
    data->edit_value_num_label = gtk_label_new(NULL);
    GtkWidget *nbox = gtk_hbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(nbox), data->edit_value_num, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(nbox), data->edit_value_num_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(xbox), nbox, FALSE, FALSE, 2);

    GtkWidget *fbuttons = gtk_hbox_new(TRUE, 4);
    GtkWidget *f_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    GtkWidget *f_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(f_apply, "clicked", G_CALLBACK(on_frame_apply), data);
    g_signal_connect(f_cancel, "clicked", G_CALLBACK(on_frame_cancel), data);
    gtk_box_pack_start(GTK_BOX(fbuttons), f_apply, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(fbuttons), f_cancel, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(xbox), fbuttons, FALSE, FALSE, 4);

    gtk_container_add(GTK_CONTAINER(data->edit_frame), xbox);

    GtkWidget *bbox = gtk_hbox_new(TRUE, 4);
    GtkWidget *btn_apply = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    GtkWidget *btn_cancel = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    g_signal_connect(btn_apply, "clicked", G_CALLBACK(on_apply_button), data);
    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(on_cancel_button), data);
    gtk_box_pack_end(GTK_BOX(bbox), btn_cancel, FALSE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(bbox), btn_apply, FALSE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), bbox, FALSE, FALSE, 4);

    gtk_widget_show_all(win);
    gtk_widget_hide(data->edit_frame);
    
    gtk_action_set_sensitive(data->edit_apply_button, gl || app);
    gtk_action_set_sensitive(data->rm_option_button, FALSE);
    gtk_action_set_sensitive(data->edit_option_button, FALSE);
}