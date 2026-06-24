/*
 * Copyright (C) 2014-2016 Andriy Grytsenko <andrej@rep.kiev.ua>
 *
 * This file is a part of LXPanel project.
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>

#include "plugin.h"
#include "gtk-compat.h"

#include <keybinder.h>

static GHashTable *all_bindings = NULL;

/* Custom signal marshaller helper dropped. 
 * We now leverage GObject's native g_cclosure_marshal_generic. */

#define PANEL_TYPE_CFG_INPUT_BUTTON     (config_input_button_get_type())

extern GType config_input_button_get_type (void) G_GNUC_CONST;

typedef struct _PanelCfgInputButton      PanelCfgInputButton;
typedef struct _PanelCfgInputButtonClass PanelCfgInputButtonClass;

struct _PanelCfgInputButton
{
    GtkFrame parent;
    GtkToggleButton *none;
    GtkToggleButton *custom;
    GtkButton *btn;
    gboolean do_key;
    gboolean do_click;
    guint key;
    GdkModifierType mods;
    gboolean has_focus;
};

struct _PanelCfgInputButtonClass
{
    GtkFrameClass parent_class;
    gboolean (*changed)(PanelCfgInputButton *btn, char *accel);
};

enum
{
    CHANGED,
    N_SIGNALS
};

static guint signals[N_SIGNALS];


/* ---- Events on test button ---- */

static gboolean on_focus_in_event(GtkWidget *test, GdkEventFocus *event,
                                  PanelCfgInputButton *btn)
{
    gtk_toggle_button_set_active(btn->custom, TRUE);
    btn->has_focus = TRUE;
    
    if (btn->do_key)
    {
        /* Modern GTK3 grab routing using GdkDevice Managers */
        GdkWindow *window = gtk_widget_get_window(test);
        if (window)
        {
            GdkDisplay *display = gdk_window_get_display(window);
            GdkSeat *seat = gdk_display_get_default_seat(display);
            gdk_seat_grab(seat, window, GDK_SEAT_CAPABILITY_KEYBOARD, TRUE,
                          NULL, (GdkEvent *)event, NULL, NULL);
        }
    }
    return FALSE;
}

static gboolean on_focus_out_event(GtkWidget *test, GdkEventFocus *event,
                                   PanelCfgInputButton *btn)
{
    btn->has_focus = FALSE;
    if (btn->do_key)
    {
        GdkWindow *window = gtk_widget_get_window(test);
        if (window)
        {
            GdkDisplay *display = gdk_window_get_display(window);
            GdkSeat *seat = gdk_display_get_default_seat(display);
            gdk_seat_ungrab(seat);
        }
    }
    return FALSE;
}

static void _button_set_click_label(GtkButton *btn, guint keyval, GdkModifierType state)
{
    char *mod_text, *text;
    const char *btn_text;
    char buff[64];

    mod_text = gtk_accelerator_get_label(0, state);
    btn_text = gdk_keyval_name(keyval);
    if (btn_text == NULL)
    {
        gtk_button_set_label(btn, "");
        g_free(mod_text);
        return;
    }
    switch (btn_text[0])
    {
    case '1':
        btn_text = _("LeftBtn");
        break;
    case '2':
        btn_text = _("MiddleBtn");
        break;
    case '3':
        btn_text = _("RightBtn");
        break;
    default:
        snprintf(buff, sizeof(buff), _("Btn%s"), btn_text);
        btn_text = buff;
    }
    text = g_strdup_printf("%s%s", mod_text, btn_text);
    gtk_button_set_label(btn, text);
    g_free(text);
    g_free(mod_text);
}

static gboolean on_key_event(GtkWidget *test, GdkEventKey *event,
                             PanelCfgInputButton *btn)
{
    GdkModifierType state;
    char *text;
    gboolean ret = FALSE;

    if (event->keyval == GDK_KEY_Tab)
        return FALSE;

    /* Fixed deprecated pointer query: extraction now handles state masks dynamically from event states */
    state = event->state;
    
    if ((state & GDK_SUPER_MASK) == 0 && (state & GDK_MOD4_MASK) != 0)
        state |= GDK_SUPER_MASK;
    state &= gtk_accelerator_get_default_mod_mask();

    if (event->is_modifier)
    {
        if (state != 0)
            text = gtk_accelerator_get_label(0, state);
        else if (btn->do_key)
            text = gtk_accelerator_get_label(btn->key, btn->mods);
        else
        {
            _button_set_click_label(GTK_BUTTON(test), btn->key, btn->mods);
            return FALSE;
        }
        gtk_button_set_label(GTK_BUTTON(test), text);
        g_free(text);
        return FALSE;
    }

    if (event->type != GDK_KEY_PRESS || !btn->do_key)
        return FALSE;

    if (state == btn->mods && event->keyval == btn->key)
    {
        text = gtk_accelerator_get_label(event->keyval, state);
        gtk_button_set_label(GTK_BUTTON(test), text);
        g_free(text);
        return FALSE;
    }

    if (state == 0 && event->keyval == GDK_KEY_BackSpace)
    {
        g_signal_emit(btn, signals[CHANGED], 0, NULL, &ret);
        if (ret)
        {
            btn->mods = 0;
            btn->key = 0;
        }
        goto _done;
    }

    if (event->length != 0 && (state == 0 || state == GDK_SHIFT_MASK ||
                               state == GDK_CONTROL_MASK || state == GDK_MOD1_MASK))
    {
        GtkWidget* dlg;
        text = gtk_accelerator_get_label(event->keyval, state);
        dlg = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                     _("Key combination '%s' cannot be used as"
                                       " a global hotkey, sorry."), text);
        g_free(text);
        gtk_window_set_title(GTK_WINDOW(dlg), _("Error"));
        gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
        
        /* Fixed blocking UI anti-pattern: converted running instance execution to async signals */
        g_signal_connect(dlg, "response", G_CALLBACK(gtk_widget_destroy), NULL);
        gtk_widget_show_all(dlg);
        return FALSE;
    }

    text = gtk_accelerator_name(event->keyval, state);
    g_signal_emit(btn, signals[CHANGED], 0, text, &ret);
    g_free(text);
    if (ret)
    {
        btn->mods = state;
        btn->key = event->keyval;
    }
_done:
    text = gtk_accelerator_get_label(btn->key, btn->mods);
    gtk_button_set_label(GTK_BUTTON(test), text);
    g_free(text);
    return FALSE;
}

static gboolean on_button_press_event(GtkWidget *test, GdkEventButton *event,
                                      PanelCfgInputButton *btn)
{
    GdkModifierType state;
    char *text;
    char digit[4];
    guint keyval;
    gboolean ret = FALSE;

    if (!btn->do_click)
        return FALSE;
        
    if (!btn->has_focus)
    {
        btn->has_focus = TRUE;
        return FALSE;
    }

    state = event->state & gtk_accelerator_get_default_mod_mask();
    if (event->button == 3 && state == 0)
        return FALSE;

    snprintf(digit, sizeof(digit), "%u", event->button);
    keyval = gdk_keyval_from_name(digit);

    if (state == btn->mods && keyval == btn->key)
    {
        _button_set_click_label(GTK_BUTTON(test), keyval, state);
        return FALSE;
    }

    text = gtk_accelerator_name(keyval, state);
    g_signal_emit(btn, signals[CHANGED], 0, text, &ret);
    g_free(text);
    if (ret)
    {
        btn->mods = state;
        btn->key = keyval;
    }
    _button_set_click_label(GTK_BUTTON(test), btn->key, btn->mods);
    return FALSE;
}

static void on_reset(GtkRadioButton *rb, PanelCfgInputButton *btn)
{
    gboolean ret = FALSE;
    if (!gtk_toggle_button_get_active(btn->none))
        return;
    btn->mods = 0;
    btn->key = 0;
    gtk_button_set_label(btn->btn, "");
    g_signal_emit(btn, signals[CHANGED], 0, NULL, &ret);
}

/* ---- Class implementation ---- */

G_DEFINE_TYPE(PanelCfgInputButton, config_input_button, GTK_TYPE_FRAME)

static void config_input_button_class_init(PanelCfgInputButtonClass *klass)
{
    /* Used generic standard marshalling block directly rather than legacy _marshal_BOOLEAN__STRING */
    signals[CHANGED] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(klass),
                     G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(PanelCfgInputButtonClass, changed),
                     g_signal_accumulator_true_handled, NULL,
                     g_cclosure_marshal_generic,
                     G_TYPE_BOOLEAN, 1, G_TYPE_STRING);
}

static void config_input_button_init(PanelCfgInputButton *self)
{
    /* Ported legacy hbox instantiation out for modern layout boxes */
    GtkWidget *w = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkBox *box = GTK_BOX(w);

    /* GtkRadioButton "None" */
    w = gtk_radio_button_new_with_label(NULL, _("None"));
    gtk_box_pack_start(box, w, FALSE, FALSE, 6);
    self->none = GTK_TOGGLE_BUTTON(w);
    gtk_toggle_button_set_active(self->none, TRUE);
    g_signal_connect(w, "toggled", G_CALLBACK(on_reset), self);
    
    /* GtkRadioButton "Custom:" */
    w = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(w), _("Custom:"));
    gtk_box_pack_start(box, w, FALSE, FALSE, 0);
    gtk_widget_set_can_focus(w, FALSE);
    self->custom = GTK_TOGGLE_BUTTON(w);
    
    /* test GtkButton */
    w = gtk_button_new_with_label(NULL);
    gtk_box_pack_start(box, w, TRUE, TRUE, 0);
    self->btn = GTK_BUTTON(w);
    gtk_button_set_label(self->btn, "        "); 
    
    g_signal_connect(w, "focus-in-event", G_CALLBACK(on_focus_in_event), self);
    g_signal_connect(w, "focus-out-event", G_CALLBACK(on_focus_out_event), self);
    g_signal_connect(w, "key-press-event", G_CALLBACK(on_key_event), self);
    g_signal_connect(w, "key-release-event", G_CALLBACK(on_key_event), self);
    g_signal_connect(w, "button-press-event", G_CALLBACK(on_button_press_event), self);
    
    w = (GtkWidget *)box;
    gtk_widget_show_all(w);
    gtk_container_add(GTK_CONTAINER(self), w);
}

static PanelCfgInputButton *_config_input_button_new(const char *label)
{
    return g_object_new(PANEL_TYPE_CFG_INPUT_BUTTON, "label", label, NULL);
}

GtkWidget *panel_config_hotkey_button_new(const char *label, const char *hotkey)
{
    PanelCfgInputButton *btn = _config_input_button_new(label);
    char *text;

    btn->do_key = TRUE;
    if (hotkey && *hotkey)
    {
        gtk_accelerator_parse(hotkey, &btn->key, &btn->mods);
        text = gtk_accelerator_get_label(btn->key, btn->mods);
        gtk_button_set_label(btn->btn, text);
        g_free(text);
        gtk_toggle_button_set_active(btn->custom, TRUE);
    }
    return GTK_WIDGET(btn);
}

GtkWidget *panel_config_click_button_new(const char *label, const char *click)
{
    PanelCfgInputButton *btn = _config_input_button_new(label);

    btn->do_click = TRUE;
    if (click && *click)
    {
        gtk_accelerator_parse(click, &btn->key, &btn->mods);
        _button_set_click_label(btn->btn, btn->key, btn->mods);
        gtk_toggle_button_set_active(btn->custom, TRUE);
    }
    return GTK_WIDGET(btn);
}

gboolean lxpanel_apply_hotkey(char **hkptr, const char *keystring,
                             void (*handler)(const char *keystring, gpointer user_data),
                             gpointer user_data, gboolean show_error)
{
    g_return_val_if_fail(hkptr != NULL, FALSE);
    g_return_val_if_fail(handler != NULL, FALSE);

    if (G_UNLIKELY(all_bindings == NULL))
        all_bindings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    if (keystring != NULL &&
        (g_hash_table_lookup(all_bindings, keystring) != NULL ||
         !keybinder_bind(keystring, handler, user_data)))
    {
        if (show_error)
        {
            GtkWidget* dlg;

            dlg = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                         _("Cannot assign '%s' as a global hotkey:"
                                           " it is already bound."), keystring);
            gtk_window_set_title(GTK_WINDOW(dlg), _("Error"));
            gtk_window_set_keep_above(GTK_WINDOW(dlg), TRUE);
            
            g_signal_connect(dlg, "response", G_CALLBACK(gtk_widget_destroy), NULL);
            gtk_widget_show_all(dlg);
        }
        return FALSE;
    }
    if (*hkptr != NULL)
    {
        keybinder_unbind(*hkptr, handler);
        if (!g_hash_table_remove(all_bindings, *hkptr))
            g_warning("%s: hotkey %s not found in hash table", __FUNCTION__, *hkptr);
    }
    *hkptr = g_strdup(keystring);
    if (*hkptr)
        g_hash_table_insert(all_bindings, *hkptr, *hkptr);
    return TRUE;
}

guint panel_config_click_parse(const char *keystring, GdkModifierType *mods)
{
    guint key;
    const char *name;

    if (keystring == NULL)
        return 0;
    gtk_accelerator_parse(keystring, &key, mods);
    name = gdk_keyval_name(key);
    if (name && name[0] >= '1' && name[0] <= '9')
        return (name[0] - '0');
    return 0;
}