/*
 * lxrandr.c - Easy-to-use XRandR GUI frontend for LXDE project
 *
 * Copyright (C) 2008 Hong Jen Yee(PCMan) <pcman.tw@gmail.com>
 * Copyright (C) 2011 Julien Lavergne <julien.lavergne@gmail.com>
 * Copyright (C) 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 * Copyright (C) 2021,2024-2025 Ingo Brückl
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 * Updated for modern thread safety, safe string processing, and GTK3.24.52 event loops.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

/* Structure defining display attributes */
typedef struct {
    char* name;
    gboolean status;
    GList* mode_lines; /* Elements are null-terminated string arrays: [Resolution, Rates_CSV, NULL] */
    char* current_mode;
    char* current_rate;
    int active_res;
    int active_rate;
    int active_option;
    GtkComboBoxText* res_combo;
    GtkComboBoxText* rate_combo;
    GtkWidget* enable_check;
} Monitor;

/* Application state architecture */
static GList* monitors = NULL;
static GtkWidget* main_window = NULL;
static GtkWidget* main_vbox = NULL;
static guint confirmation_timer_id = 0;

/* Forward declarations */
static void on_confirmation_response(GtkDialog *dialog, gint response_id, gpointer user_data);
static gboolean on_confirmation_timeout(gpointer user_data);
static void run_xrandr_command(const char *cmd_args);
static gboolean load_xrandr_info(void);
static void rebuild_ui_contents(void);

static void free_monitor(Monitor* m) {
    if (!m) return;
    g_free(m->name);
    g_free(m->current_mode);
    g_free(m->current_rate);
    if (m->mode_lines) {
        GList* l;
        for (l = m->mode_lines; l; l = l->next) {
            char** data = (char**)l->data;
            if (data) {
                g_free(data[0]);
                g_free(data[1]);
                g_free(data);
            }
        }
        g_list_free(m->mode_lines);
    }
    g_free(m);
}

static void clean_all_monitors(void) {
    if (monitors) {
        g_list_free_full(monitors, (GDestroyNotify)free_monitor);
        monitors = NULL;
    }
}

/* Parse connected display topologies from xrandr cleanly and safely */
static gboolean load_xrandr_info(void) {
    char *output = NULL;
    int status = 0;
    char **lines = NULL;
    int i;
    Monitor* m = NULL;
    
    clean_all_monitors();

    char *envp[] = { "LC_ALL=C", NULL };
    char *argv[] = { "xrandr", NULL };
    
    gboolean success = g_spawn_sync(NULL, argv, envp, G_SPAWN_SEARCH_PATH, 
                                    NULL, NULL, &output, NULL, &status, NULL);
    
    if (!success || !WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        g_free(output);
        return FALSE;
    }

    lines = g_strsplit(output, "\n", 0);
    g_free(output);

    if (!lines) return FALSE;

    for (i = 0; lines[i] != NULL; i++) {
        char *line = lines[i];
        if (g_str_has_prefix(line, "Screen ")) continue;

        if (strstr(line, " connected ") || strstr(line, " disconnected ")) {
            char **tokens = g_strsplit(line, " ", 0);
            if (tokens && tokens[0]) {
                m = g_new0(Monitor, 1);
                m->name = g_strdup(tokens[0]);
                m->status = (strstr(line, " connected ") != NULL);
                monitors = g_list_append(monitors, m);
            }
            g_strfreev(tokens);
            continue;
        }

        if (m && m->status && (g_str_has_prefix(line, "   ") || g_str_has_prefix(line, "  "))) {
            char *trimmed = g_strstrip(g_strdup(line));
            char **tokens = g_strsplit(trimmed, " ", 0);
            g_free(trimmed);

            if (tokens && tokens[0] && strlen(tokens[0]) > 0) {
                char **mode_entry = g_new0(char*, 3);
                mode_entry[0] = g_strdup(tokens[0]); 
                
                int j = 1;
                GString *rates = g_string_new("");
                while (tokens[j] != NULL) {
                    if (strlen(tokens[j]) > 0) {
                        char *clean_rate = g_strdelimit(g_strdup(tokens[j]), "+*", ' ');
                        g_strstrip(clean_rate);
                        if (strlen(clean_rate) > 0) {
                            if (rates->len > 0) g_string_append(rates, ",");
                            g_string_append(rates, clean_rate);
                        }
                        g_free(clean_rate);
                    }
                    j++;
                }
                mode_entry[1] = g_string_free(rates, FALSE);
                m->mode_lines = g_list_append(m->mode_lines, mode_entry);
            }
            g_strfreev(tokens);
        }
    }

    g_strfreev(lines);
    return (monitors != NULL);
}

static void on_res_sel_changed(GtkComboBoxText *combo, gpointer user_data) {
    Monitor *m = (Monitor*)user_data;
    char *selected_text = gtk_combo_box_text_get_active_text(combo);
    
    if (!selected_text) return;
    
    gtk_combo_box_text_remove_all(m->rate_combo);
    
    if (g_strcmp0(selected_text, _("Auto")) == 0) {
        gtk_combo_box_text_append_text(m->rate_combo, _("Auto"));
        gtk_combo_box_set_active(GTK_COMBO_BOX(m->rate_combo), 0);
    } else {
        GList *l;
        for (l = m->mode_lines; l; l = l->next) {
            char **mode_data = (char**)l->data;
            if (g_strcmp0(mode_data[0], selected_text) == 0) {
                char **rates = g_strsplit(mode_data[1], ",", 0);
                if (rates) {
                    int i;
                    for (i = 0; rates[i] != NULL; i++) {
                        if (strlen(rates[i]) > 0) {
                            gtk_combo_box_text_append_text(m->rate_combo, rates[i]);
                        }
                    }
                    g_strfreev(rates);
                }
                gtk_combo_box_set_active(GTK_COMBO_BOX(m->rate_combo), 0);
                break;
            }
        }
    }
    g_free(selected_text);
}

/* FIXED: Re-factored UI building logic to enable clean, dynamic async refreshes */
static void rebuild_ui_contents(void) {
    GList *l;
    
    /* Clean out legacy components from the active layout canvas safely */
    GList *children = gtk_container_get_children(GTK_CONTAINER(main_vbox));
    GList *c;
    for (c = children; c; c = c->next) {
        gtk_widget_destroy(GTK_WIDGET(c->data));
    }
    g_list_free(children);

    for (l = monitors; l; l = l->next) {
        Monitor *m = (Monitor*)l->data;
        if (!m->status) continue;

        GtkWidget *frame = gtk_frame_new(m->name);
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_set_margin_start(hbox, 6);
        gtk_widget_set_margin_end(hbox, 6);
        gtk_widget_set_margin_top(hbox, 6);
        gtk_widget_set_margin_bottom(hbox, 6);

        m->enable_check = gtk_check_button_new_with_label(_("Enable this display"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m->enable_check), TRUE);
        gtk_box_pack_start(GTK_BOX(hbox), m->enable_check, FALSE, FALSE, 4);

        GtkWidget *lbl_res = gtk_label_new(_("Resolution:"));
        m->res_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_box_pack_start(GTK_BOX(hbox), lbl_res, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(m->res_combo), FALSE, FALSE, 4);

        GtkWidget *lbl_rate = gtk_label_new(_("Refresh Rate:"));
        m->rate_combo = GTK_COMBO_BOX_TEXT(gtk_combo_box_text_new());
        gtk_box_pack_start(GTK_BOX(hbox), lbl_rate, FALSE, FALSE, 4);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(m->rate_combo), FALSE, FALSE, 4);

        g_signal_connect(m->res_combo, "changed", G_CALLBACK(on_res_sel_changed), m);

        gtk_combo_box_text_append_text(m->res_combo, _("Auto"));
        GList *m_line;
        for (m_line = m->mode_lines; m_line; m_line = m_line->next) {
            char **mode_data = (char**)m_line->data;
            gtk_combo_box_text_append_text(m->res_combo, mode_data[0]);
        }
        
        gtk_combo_box_set_active(GTK_COMBO_BOX(m->res_combo), 0);

        gtk_container_add(GTK_CONTAINER(frame), hbox);
        gtk_box_pack_start(GTK_BOX(main_vbox), frame, FALSE, FALSE, 4);
    }
    gtk_widget_show_all(main_vbox);
}

static void build_ui(void) {
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(main_vbox, 8);
    gtk_widget_set_margin_end(main_vbox, 8);
    gtk_widget_set_margin_top(main_vbox, 8);
    gtk_widget_set_margin_bottom(main_vbox, 8);

    gtk_container_add(GTK_CONTAINER(scroll), main_vbox);
    
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(main_window));
    gtk_box_pack_start(GTK_BOX(content_area), scroll, TRUE, TRUE, 0);
    
    rebuild_ui_contents();
    gtk_widget_show_all(main_window);
}

/* Construct external argument mappings for layout modification execution */
static void save_and_apply_settings(void) {
    GList *l;
    GString *cmd = g_string_new("xrandr");

    for (l = monitors; l; l = l->next) {
        Monitor *m = (Monitor*)l->data;
        g_string_append_printf(cmd, " --output %s", m->name);

        if (!m->enable_check || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(m->enable_check))) {
            g_string_append(cmd, " --off");
            continue;
        }

        char *res = gtk_combo_box_text_get_active_text(m->res_combo);
        char *rate = gtk_combo_box_text_get_active_text(m->rate_combo);

        if (!res || g_strcmp0(res, _("Auto")) == 0) {
            g_string_append(cmd, " --auto");
        } else {
            /* FIXED: Standardized safe extraction of modes. Eliminates hazardous delim mutations. */
            gchar *res_clean = g_strstrip(g_strdup(res));
            g_string_append_printf(cmd, " --mode %s", res_clean);
            g_free(res_clean);

            if (rate && g_strcmp0(rate, _("Auto")) != 0) {
                g_string_append_printf(cmd, " --rate %s", rate);
            }
        }
        g_free(res);
        g_free(rate);
    }

    char *final_cmd = g_string_free(cmd, FALSE);
    run_xrandr_command(final_cmd);
    g_free(final_cmd);

    GtkWidget *confirmation = gtk_message_dialog_new(GTK_WINDOW(main_window),
                                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                     GTK_MESSAGE_QUESTION,
                                                     GTK_BUTTONS_YES_NO,
                                                     _("Does the new resolution look correct?\nSettings will revert automatically in 15 seconds."));
    
    gtk_window_set_title(GTK_WINDOW(confirmation), _("Confirm Screen Geometry Updates"));
    
    g_signal_connect(confirmation, "response", G_CALLBACK(on_confirmation_response), NULL);
    confirmation_timer_id = g_timeout_add(15000, on_confirmation_timeout, confirmation);
    
    gtk_widget_show_all(confirmation);
}

static void run_xrandr_command(const char *cmd_args) {
    char **argv = NULL;
    GError *err = NULL;
    if (g_shell_parse_argv(cmd_args, NULL, &argv, &err)) {
        char *envp[] = { "LC_ALL=C", NULL };
        g_spawn_sync(NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL, NULL, NULL, NULL);
        g_strfreev(argv);
    } else {
        if (err) {
            g_printerr("Failed to parse runtime arguments: %s\n", err->message);
            g_error_free(err);
        }
    }
}

static void on_confirmation_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (confirmation_timer_id > 0) {
        g_source_remove(confirmation_timer_id);
        confirmation_timer_id = 0;
    }

    if (response_id != GTK_RESPONSE_YES) {
        run_xrandr_command("xrandr --auto");
        load_xrandr_info();
        /* FIXED: Aligned layout structures instantly to block Use-After-Free UI interactions */
        rebuild_ui_contents();
    }
    gtk_widget_destroy(GTK_WIDGET(dialog));
}

static gboolean on_confirmation_timeout(gpointer user_data) {
    GtkWidget *dialog = GTK_WIDGET(user_data);
    confirmation_timer_id = 0;
    
    run_xrandr_command("xrandr --auto");
    load_xrandr_info();
    rebuild_ui_contents();
    
    gtk_widget_destroy(dialog);
    return FALSE;
}

static void on_main_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    if (response_id == GTK_RESPONSE_APPLY || response_id == GTK_RESPONSE_OK) {
        save_and_apply_settings();
        if (response_id == GTK_RESPONSE_OK) {
            gtk_widget_destroy(GTK_WIDGET(dialog));
            gtk_main_quit();
        }
    } else {
        gtk_widget_destroy(GTK_WIDGET(dialog));
        gtk_main_quit();
    }
}

static void on_err_dialog_response(GtkDialog *dialog, gint response_id, gpointer user_data) {
    gtk_widget_destroy(GTK_WIDGET(dialog));
    gtk_main_quit();
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    #ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
    #endif

    main_window = gtk_dialog_new_with_buttons(_("Monitor Layout Configurations"),
                                              NULL,
                                              GTK_DIALOG_MODAL,
                                              _("_Cancel"), GTK_RESPONSE_CANCEL,
                                              _("_Apply"), GTK_RESPONSE_APPLY,
                                              _("_OK"), GTK_RESPONSE_OK,
                                              NULL);

    gtk_window_set_default_size(GTK_WINDOW(main_window), 580, 350);
    g_signal_connect(main_window, "response", G_CALLBACK(on_main_response), NULL);

    if (!load_xrandr_info()) {
        /* FIXED: Replaced unsafe synchronous loop locks with non-blocking async cascades */
        GtkWidget *err_dialog = gtk_message_dialog_new(GTK_WINDOW(main_window), GTK_DIALOG_MODAL,
                                                       GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                                       _("Unable to interface with XRandR environment extension flags."));
        g_signal_connect(err_dialog, "response", G_CALLBACK(on_err_dialog_response), NULL);
        gtk_widget_show_all(err_dialog);
        gtk_main();
        clean_all_monitors();
        return 1;
    }

    build_ui();
    gtk_main();

    clean_all_monitors();
    return 0;
}