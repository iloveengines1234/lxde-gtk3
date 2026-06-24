/**
 *      Copyright 2008 Fred Chien <cfsghost@gmail.com>
 *      Copyright (c) 2010 LxDE Developers, see the file AUTHORS for details.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <vte/vte.h>
#include <langinfo.h>
#include <locale.h>
#include <sys/stat.h>
#include <pwd.h>
#include <limits.h>
#include <string.h>

#if VTE_CHECK_VERSION (0, 46, 0)
# define PCRE2_CODE_UNIT_WIDTH 0
# include <pcre2.h>
#endif

#include "lxterminal.h"
#include "setting.h"
#include "preferences.h"
#include "unixsocket.h"

/* Forward Declarations of Utility Pipelines */
static void terminal_get_border(Term *term, GtkBorder *border);
static void terminal_save_size(LXTerminal *terminal);
static void terminal_tab_set_position(GtkWidget *notebook, gint tab_position);
static gchar *terminal_get_current_dir(LXTerminal *terminal);
static const gchar *terminal_get_preferred_shell(void);

/* Menu and Accelerator Interaction Signal Actions */
static void terminal_initialize_switch_tab_accelerator(Term *term);
static void terminal_update_alt(LXTerminal *terminal);
static gboolean terminal_switch_tab_accelerator(Term *term);
static void terminal_new_window_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_new_tab_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_close_tab_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_close_window_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_open_url_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_copy_url_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_copy_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_paste_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_clear_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_name_tab_response_event(GtkWidget *dialog, gint response, Term *term);
static void terminal_name_tab_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_previous_tab_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_next_tab_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_move_tab_execute(LXTerminal *terminal, gint direction);
static void terminal_move_tab_left_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_move_tab_right_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_zoom(LXTerminal *terminal);
static gboolean terminal_zoom_in_activate_event(GtkAction *action, LXTerminal *terminal);
static gboolean terminal_zoom_out_activate_event(GtkAction *action, LXTerminal *terminal);
static gboolean terminal_zoom_reset_activate_event(GtkAction *action, LXTerminal *terminal);
static void terminal_about_activate_event(GtkAction *action, LXTerminal *terminal);

/* Windows Geometry and Execution Management Maps */
static void terminal_switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint num, LXTerminal *terminal);
static void terminal_vte_size_allocate_event(GtkWidget *widget, GtkAllocation *allocation, Term *term);
static void terminal_window_title_changed_event(GtkWidget *vte, Term *term);
static gboolean terminal_close_window_confirmation_event(GtkWidget *widget, GdkEventButton *event, LXTerminal *terminal);
static gboolean terminal_close_window_confirmation_dialog(LXTerminal *terminal);
static void terminal_window_exit(LXTerminal *terminal, GObject *where_the_object_was);
#if VTE_CHECK_VERSION (0, 38, 0)
static void terminal_child_exited_event(VteTerminal *vte, gint status, Term *term);
#else
static void terminal_child_exited_event(VteTerminal *vte, Term *term);
#endif
static void terminal_close_button_event(GtkButton *button, Term *term);
static gboolean terminal_tab_button_press_event(GtkWidget *widget, GdkEventButton *event, Term *term);
static void terminal_vte_cursor_moved_event(VteTerminal *vte, Term *term);
static void terminal_vte_visual_bell(VteTerminal *vte, Term *term);
static gboolean terminal_vte_button_press_event(VteTerminal *vte, GdkEventButton *event, Term *term);
static void terminal_settings_apply_to_term(LXTerminal *terminal, Term *term);
static Term *terminal_new(LXTerminal *terminal, const gchar *label, const gchar *pwd, gchar **env, gchar **exec);
static void terminal_set_geometry_hints(Term *term, GdkGeometry *geometry);
static void terminal_new_tab(LXTerminal *terminal, const gchar *label);
static void terminal_free(Term *term);
static void terminal_menubar_initialize(LXTerminal *terminal);
static void terminal_menu_accelerator_update(LXTerminal *terminal);
static void terminal_settings_apply(LXTerminal *terminal);
static void terminal_update_menu_shortcuts(Setting *setting);
static void terminal_initialize_menu_shortcuts(Setting *setting);

/* State Cache Storage Tracker */
static char *saved_menu_accelerator = NULL;

/* Command Line Parsing Interface Manual Guidelines */
static const gchar usage_display[] = 
    "Usage:\n"
    "  lxterminal [Options...] - LXTerminal is a terminal emulator\n\n"
    "Options:\n"
    "  -e, --command=STRING             Execute the argument to this option inside the terminal\n"
    "  --geometry=COLUMNSxROWS          Set the terminal's size\n"
    "  -l, --loginshell                 Execute login shell\n"
    "  -t, -T, --title=,\n"
    "    --tabs=NAME[,NAME[,NAME[...]]] Set the terminal's title\n"
    "  --working-directory=DIRECTORY    Set the terminal's working directory\n"
    "  --no-remote                      Do not accept or send remote commands\n"
    "  --profile=NAME                   Use a separate configuration profile\n"
    "  -v, --version                    Version information\n";

/* Action Array Configuration Map for Top Menubar Modules */
static const GtkActionEntry menu_items[] =
{
    { "File", NULL, N_("_File"), NULL, NULL, NULL },
    { "Edit", NULL, N_("_Edit"), NULL, NULL, NULL },
    { "Tabs", NULL, N_("_Tabs"), NULL, NULL, NULL },
    { "Help", NULL, N_("_Help"), NULL, NULL, NULL },
    { "File_NewWindow", "list-add", N_("_New Window"), NEW_WINDOW_ACCEL_DEF, "New Window", G_CALLBACK(terminal_new_window_activate_event) },
    { "File_NewTab", "list-add", N_("New T_ab"), NEW_TAB_ACCEL_DEF, "New Tab", G_CALLBACK(terminal_new_tab_activate_event) },
    { "File_Sep1", NULL, "Sep", NULL, NULL, NULL },
    { "File_CloseTab", "window-close", N_("_Close Tab"), CLOSE_TAB_ACCEL_DEF, "Close Tab", G_CALLBACK(terminal_close_tab_activate_event) },
    { "File_CloseWindow", "application-exit", N_("Close _Window"), CLOSE_WINDOW_ACCEL_DEF, "Close Window", G_CALLBACK(terminal_close_window_activate_event) },
    { "Edit_Copy", "edit-copy", N_("Cop_y"), COPY_ACCEL_DEF, "Copy", G_CALLBACK(terminal_copy_activate_event) },
    { "Edit_Paste", "edit-paste", N_("_Paste"), PASTE_ACCEL_DEF, "Paste", G_CALLBACK(terminal_paste_activate_event) },
    { "Edit_Clear", NULL, N_("Clear scr_ollback"), NULL, "Clear scrollback", G_CALLBACK(terminal_clear_activate_event) },
    { "Edit_Sep1", NULL, "Sep", NULL, NULL, NULL },
    { "Edit_ZoomIn", "zoom-in", N_("Zoom _In"), ZOOM_IN_ACCEL_DEF, "Zoom In", G_CALLBACK(terminal_zoom_in_activate_event) },
    { "Edit_ZoomOut", "zoom-out", N_("Zoom O_ut"), ZOOM_OUT_ACCEL_DEF, "Zoom Out", G_CALLBACK(terminal_zoom_out_activate_event) },
    { "Edit_ZoomReset", "zoom-fit-best", N_("Zoom _Reset"), ZOOM_RESET_ACCEL_DEF, "Zoom Reset", G_CALLBACK(terminal_zoom_reset_activate_event) },
    { "Edit_Sep2", NULL, "Sep", NULL, NULL, NULL },
    { "Edit_Preferences", "system-run", N_("Preference_s"), NULL, "Preferences", G_CALLBACK(terminal_preferences_dialog) },
    { "Tabs_NameTab", "dialog-information", N_("Na_me Tab"), NAME_TAB_ACCEL_DEF, "Name Tab", G_CALLBACK(terminal_name_tab_activate_event) },
    { "Tabs_PreviousTab", "go-previous", N_("Pre_vious Tab"), PREVIOUS_TAB_ACCEL_DEF, "Previous Tab", G_CALLBACK(terminal_previous_tab_activate_event) },
    { "Tabs_NextTab", "go-next", N_("Ne_xt Tab"), NEXT_TAB_ACCEL_DEF, "Next Tab", G_CALLBACK(terminal_next_tab_activate_event) },
    { "Tabs_MoveTabLeft", NULL, N_("Move Tab _Left"), MOVE_TAB_LEFT_ACCEL_DEF, "Move Tab Left", G_CALLBACK(terminal_move_tab_left_activate_event) },
    { "Tabs_MoveTabRight", NULL, N_("Move Tab _Right"), MOVE_TAB_RIGHT_ACCEL_DEF, "Move Tab Right", G_CALLBACK(terminal_move_tab_right_activate_event) },
    { "Help_About", "help-about", N_("_About"), NULL, "About", G_CALLBACK(terminal_about_activate_event) }
};
#define MENUBAR_MENUITEM_COUNT G_N_ELEMENTS(menu_items)

/* Action Entry Configuration Map for Right-Click Context Popup Overlays */
static const GtkActionEntry vte_menu_items[] =
{
    { "VTEMenu", NULL, "VTEMenu", NULL, NULL, NULL },
    { "NewWindow", "list-add", N_("New _Window"), NULL, "New Window", G_CALLBACK(terminal_new_window_activate_event) },
    { "NewTab", "list-add", N_("New _Tab"), NULL, "New Tab", G_CALLBACK(terminal_new_tab_activate_event) },
    { "Sep1", NULL, "Sep", NULL, NULL, NULL },
    { "OpenURL", NULL, N_("Open _URL"), NULL, "Open URL", G_CALLBACK(terminal_open_url_activate_event) },
    { "CopyURL", NULL, N_("Copy _URL"), NULL, "Copy URL", G_CALLBACK(terminal_copy_url_activate_event) },
    { "Copy", "edit-copy", N_("Cop_y"), NULL, "Copy", G_CALLBACK(terminal_copy_activate_event) },
    { "Paste", "edit-paste", N_("_Paste"), NULL, "Paste", G_CALLBACK(terminal_paste_activate_event) },
    { "Clear", NULL, N_("Cl_ear scrollback"), NULL, "Clear scrollback", G_CALLBACK(terminal_clear_activate_event) },
    { "Sep2", NULL, "Sep", NULL, NULL, NULL },
    { "Preferences", "system-run", N_("Preference_s"), NULL, "Preferences", G_CALLBACK(terminal_preferences_dialog) },
    { "Sep3", NULL, "Sep", NULL, NULL, NULL },
    { "NameTab", "dialog-information", N_("Na_me Tab"), NULL, "Name Tab", G_CALLBACK(terminal_name_tab_activate_event) },
    { "PreviousTab", "go-previous", N_("Pre_vious Tab"), NULL, "Previous Tab", G_CALLBACK(terminal_previous_tab_activate_event) },
    { "NextTab", "go-next", N_("Ne_xt Tab"), NULL, "Next Tab", G_CALLBACK(terminal_next_tab_activate_event) },
    { "Tabs_MoveTabLeft", NULL, N_("Move Tab _Left"), NULL, "Move Tab Left", G_CALLBACK(terminal_move_tab_left_activate_event) },
    { "Tabs_MoveTabRight", NULL, N_("Move Tab _Right"), NULL, "Move Tab Right", G_CALLBACK(terminal_move_tab_right_activate_event) },
    { "CloseTab", "window-close", N_("_Close Tab"), NULL, "Close Tab", G_CALLBACK(terminal_close_tab_activate_event) }
};
#define VTE_MENUITEM_COUNT G_N_ELEMENTS(vte_menu_items)

/* Thread-Safe Runtime Extraction of VteTerminal Padding Bounds */
static void terminal_get_border(Term *term, GtkBorder *border)
{
#if VTE_CHECK_VERSION (0, 38, 0)
    GtkBorder padding;
    gtk_style_context_get_padding(gtk_widget_get_style_context(term->vte),
                                  gtk_widget_get_state_flags(term->vte),
                                  &padding);
    border->left = padding.left;
    border->right = padding.right;
    border->top = padding.top;
    border->bottom = padding.bottom;
#elif VTE_CHECK_VERSION(0, 24, 0)
    GtkBorder *_border = NULL;
    gtk_widget_style_get(term->vte, "inner-border", &_border, NULL);
    if (_border) {
        memcpy(border, _border, sizeof(GtkBorder));
        gtk_border_free(_border);
    } else {
        border->left = border->right = border->top = border->bottom = 1;
    }
#else
    vte_terminal_get_padding(VTE_TERMINAL(term->vte), &border->left, &border->top);
    border->right = border->left;
    border->bottom = border->top;
#endif
}

/* Notebook Panel Alignment Direction Controller */
static void terminal_tab_set_position(GtkWidget *notebook, gint tab_position)
{
    GtkPositionType position;
    switch (tab_position)
    {
        case 0:  position = GTK_POS_TOP;    break;
        case 1:  position = GTK_POS_BOTTOM; break;
        case 2:  position = GTK_POS_LEFT;   break;
        default: position = GTK_POS_RIGHT;  break;
    }
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), position);
}

/* Process Workspace Identifier Engine Query Mapping Pipeline */
static gchar *terminal_get_current_dir(LXTerminal *terminal)
{
    gchar *proc_cwd = NULL;

#ifdef __linux__
    gint current = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current != -1)
    {
        guint i;
        for (i = 0; i < terminal->terms->len; i++)
        {
            Term *term = g_ptr_array_index(terminal->terms, i);
            if (term && term->index == current)
            {
                gchar proc_cwd_link[PATH_MAX];
                g_snprintf(proc_cwd_link, sizeof(proc_cwd_link), "/proc/%d/cwd", term->pid);
                proc_cwd = g_file_read_link(proc_cwd_link, NULL);
                break;
            }
        }
    }
#endif

    if (proc_cwd == NULL)
    {
        proc_cwd = g_get_current_dir();
    }

    return proc_cwd;
}

/* Platform Host Shell Configuration Discovery Fallback Framework */
static const gchar *terminal_get_preferred_shell(void)
{
    const gchar *shell = g_getenv("SHELL");
    if (geteuid() == getuid() && getegid() == getgid()) {
        if (shell != NULL && access(shell, X_OK) == 0) {
            return shell;
        }
    }

    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_shell && access(pw->pw_shell, X_OK) == 0) {
        return pw->pw_shell;
    }

    const gchar *fallback_shell = "/bin/sh";
    if (access(fallback_shell, X_OK) == 0) {
        return fallback_shell;
    }

    return NULL;
}

/* Registers Dynamic <ALT>+Digit Window Switching Short-keys Mapping */
static void terminal_initialize_switch_tab_accelerator(Term *term)
{
    if ((term->index + 1) < 10)
    {
        char switch_tab_accel[32];
        g_snprintf(switch_tab_accel, sizeof(switch_tab_accel), "<ALT>%d", term->index + 1);

        guint key;
        GdkModifierType mods;
        gtk_accelerator_parse(switch_tab_accel, &key, &mods);

        term->closure = g_cclosure_new_swap(G_CALLBACK(terminal_switch_tab_accelerator), term, NULL);
        if (gtk_accel_group_from_accel_closure(term->closure) == NULL)
        {
            gtk_accel_group_connect(term->parent->accel_group, key, mods, GTK_ACCEL_LOCKED, term->closure);
        }
    }
}

/* Toggles or Flushes System Wide Navigation Keys on Event Invalidation */
void terminal_update_alt(LXTerminal *terminal)
{
    guint i;
    Term *term;

    if (get_setting()->disable_alt || terminal->terms->len <= 1)
    {
        for (i = 0; i < terminal->terms->len; i++)
        {
            term = g_ptr_array_index(terminal->terms, i);
            if (term && term->closure != NULL)
            {
                gtk_accel_group_disconnect(terminal->accel_group, term->closure);
                term->closure = NULL;
            }
        }
    }
    else
    {
        for (i = 0; i < terminal->terms->len; i++)
        {
            term = g_ptr_array_index(terminal->terms, i);
            if (term && GTK_IS_ACCEL_GROUP(terminal->accel_group))
            {
                if (term->closure != NULL)
                {
                    gtk_accel_group_disconnect(terminal->accel_group, term->closure);
                    term->closure = NULL;
                }
                terminal_initialize_switch_tab_accelerator(term);
            }
        }
    }
}

static gboolean terminal_switch_tab_accelerator(Term *term)
{
    LXTerminal *terminal = term->parent;
    if (term->index < gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal->notebook))) {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal->notebook), term->index);
        return TRUE;
    }
    return FALSE;
}

static void terminal_new_window_activate_event(GtkAction *action, LXTerminal *terminal)
{
    CommandArguments arguments;
    memset(&arguments, 0, sizeof(arguments));
    arguments.working_directory = terminal_get_current_dir(terminal);
    lxterminal_initialize(terminal->parent, &arguments);
    g_free(arguments.working_directory);
}

static void terminal_new_tab_activate_event(GtkAction *action, LXTerminal *terminal)
{
    terminal_new_tab(terminal, NULL);
}

static void terminal_set_geometry_hints(Term *term, GdkGeometry *geometry)
{
    VteTerminal *vteterm = VTE_TERMINAL(term->vte);
    gint windowwidth, windowheight;
    GtkAllocation termAlloc;
    
    gtk_widget_get_allocation(GTK_WIDGET(term->vte), &termAlloc);
    gint charwidth = vte_terminal_get_char_width(vteterm);
    gint charheight = vte_terminal_get_char_height(vteterm);
    
    gtk_window_get_size(GTK_WINDOW(term->parent->window), &windowwidth, &windowheight);
    GtkBorder termBorder;
    terminal_get_border(term, &termBorder);

    gint borderwidth = windowwidth - termAlloc.width + termBorder.left + termBorder.right;
    gint borderheight = windowheight - termAlloc.height + termBorder.top + termBorder.bottom;

    geometry->min_width = borderwidth + charwidth * 6;
    geometry->min_height = borderheight + charheight * 3;
    geometry->base_width = borderwidth;
    geometry->base_height = borderheight;
    geometry->width_inc = charwidth;
    geometry->height_inc = charheight;

    gtk_window_set_geometry_hints(GTK_WINDOW(term->parent->window), NULL, geometry,
                                  GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE | GDK_HINT_RESIZE_INC);
}

static void terminal_save_size(LXTerminal *terminal)
{
    Term *term = g_ptr_array_index(terminal->terms, 0);
    if (term) {
        terminal->col = vte_terminal_get_column_count(VTE_TERMINAL(term->vte));
        terminal->row = vte_terminal_get_row_count(VTE_TERMINAL(term->vte));
    }
}

static void terminal_new_tab(LXTerminal *terminal, const gchar *label)
{
    Term *term;
    gchar *proc_cwd = terminal_get_current_dir(terminal);

    if (terminal->login_shell)
    {
        gchar **exec = g_malloc0(3 * sizeof(gchar *));
        exec[0] = g_strdup(terminal_get_preferred_shell());
        char *shellname = g_path_get_basename(exec[0]);
        exec[1] = g_strdup_printf("-%s", shellname);
        g_free(shellname);
        exec[2] = NULL;
        term = terminal_new(terminal, label, proc_cwd, NULL, exec);
    }
    else
    {
        term = terminal_new(terminal, label, proc_cwd, NULL, NULL);
    }
    g_free(proc_cwd);

    if (!term) return;

    gtk_notebook_append_page(GTK_NOTEBOOK(terminal->notebook), term->box, term->tab);
    term->index = gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal->notebook)) - 1;
    g_ptr_array_add(terminal->terms, term);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal->notebook), term->index);
    if (term->index > 0)
    {
        terminal_save_size(terminal);
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(terminal->notebook), TRUE);
    }

    terminal_update_alt(terminal);
}

static void terminal_vte_size_allocate_event(GtkWidget *widget, GtkAllocation *allocation, Term *term)
{
    GdkGeometry geometry;
    terminal_set_geometry_hints(term, &geometry);
    g_signal_handlers_disconnect_by_func(widget, G_CALLBACK(terminal_vte_size_allocate_event), term);
}

static void terminal_close_tab_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page >= 0 && (guint)current_page < terminal->terms->len) {
        Term *term = g_ptr_array_index(terminal->terms, current_page);
        if (term) {
#if VTE_CHECK_VERSION (0, 38, 0)
            terminal_child_exited_event(VTE_TERMINAL(term->vte), 0, term);
#else
            terminal_child_exited_event(VTE_TERMINAL(term->vte), term);
#endif
        }
    }
}

static void terminal_close_window_activate_event(GtkAction *action, LXTerminal *terminal)
{
    if (!terminal_close_window_confirmation_dialog(terminal)) {
        return;
    }

    /* Iterate backward safely to prevent array shifting edge cases */
    gint i;
    for (i = (gint)terminal->terms->len - 1; i >= 0; i--) {
        Term *term = g_ptr_array_index(terminal->terms, i);
        if (term) {
#if VTE_CHECK_VERSION (0, 38, 0)
            terminal_child_exited_event(VTE_TERMINAL(term->vte), 0, term);
#else
            terminal_child_exited_event(VTE_TERMINAL(term->vte), term);
#endif
        }
    }
}
/* Handler for the "Open URL" right-click menu item.
 * Fork+exec xdg-open the url. */
static void terminal_open_url_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page < 0 || (guint)current_page >= terminal->terms->len) {
        return;
    }

    Term *term = g_ptr_array_index(terminal->terms, current_page);
    if (term && term->matched_url)
    {
        gchar *cmd = g_strdup_printf("xdg-open %s", term->matched_url);
        if (!g_spawn_command_line_async(cmd, NULL)) {
            g_warning("Failed to launch xdg-open. The command was `%s'\n", cmd);
        }
        g_free(cmd);
    }
}

/* Handler for the "Copy URL" right-click menu item.
 * Copy the URL to the clipboard. */
static void terminal_copy_url_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page < 0 || (guint)current_page >= terminal->terms->len) {
        return;
    }

    Term *term = g_ptr_array_index(terminal->terms, current_page);
    if (term && term->matched_url)
    {
        GtkClipboard *clipboard = gtk_clipboard_get(gdk_atom_intern("CLIPBOARD", FALSE));
        gtk_clipboard_set_text(clipboard, term->matched_url, -1);
    }
}

/* Handler for "activate" signal on Edit/Copy menu item.
 * Copy to the clipboard. */
static void terminal_copy_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page < 0 || (guint)current_page >= terminal->terms->len) {
        return;
    }

    Term *term = g_ptr_array_index(terminal->terms, current_page);
    if (term) {
#if VTE_CHECK_VERSION (0, 50, 0)
        vte_terminal_copy_clipboard_format(VTE_TERMINAL(term->vte), VTE_FORMAT_TEXT);
#else
        vte_terminal_copy_clipboard(VTE_TERMINAL(term->vte));
#endif
    }
}

/* Handler for "activate" signal on Edit/Paste menu item.
 * Paste from the clipboard. */
static void terminal_paste_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page < 0 || (guint)current_page >= terminal->terms->len) {
        return;
    }

    Term *term = g_ptr_array_index(terminal->terms, current_page);
    if (term) {
        vte_terminal_paste_clipboard(VTE_TERMINAL(term->vte));
    }
}

/* Handler for "clear scrollback" signal on Edit/Clear menu item.
 * Clear scrollback buffer loops. */
static void terminal_clear_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page < 0 || (guint)current_page >= terminal->terms->len) {
        return;
    }

    Term *term = g_ptr_array_index(terminal->terms, current_page);
    if (term) {
        vte_terminal_set_scrollback_lines(VTE_TERMINAL(term->vte), 0);
        vte_terminal_set_scrollback_lines(VTE_TERMINAL(term->vte), get_setting()->scrollback);
    }
}

/* Handler for "response" signal on Name Tab dialog. */
static void terminal_name_tab_response_event(GtkWidget *dialog, gint response, Term *term)
{
    if (response == GTK_RESPONSE_OK)
    {
        GtkWidget *dialog_item = g_object_get_data(G_OBJECT(dialog), "entry");
        const gchar *title = NULL;

        if (dialog_item && GTK_IS_ENTRY(dialog_item))
        {
            /* Set the tab's label and mark it so we will never overwrite it automatically. */
            if (gtk_entry_get_text_length(GTK_ENTRY(dialog_item)) != 0)
            {
                term->user_specified_label = TRUE;
                title = gtk_entry_get_text(GTK_ENTRY(dialog_item));
                gtk_label_set_text(GTK_LABEL(term->label), title);
            }
            else
            {
                /* Reset tab label if it is cleared out to an empty string */
                term->user_specified_label = FALSE;
                title = vte_terminal_get_window_title(VTE_TERMINAL(term->vte));
                gtk_label_set_text(GTK_LABEL(term->label), title ? title : "");
            }

            /* Set title if term is currently the active tab in focus */
            gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(term->parent->notebook));
            if (current_page >= 0 && GTK_WIDGET(term->box) == gtk_notebook_get_nth_page(GTK_NOTEBOOK(term->parent->notebook), current_page)) {
                gtk_window_set_title(GTK_WINDOW(term->parent->window), title ? title : _("LXTerminal"));
            }
        }
    }

    /* Dismiss name modification dialog window */
    gtk_widget_destroy(dialog);
}

/* Handler for "activate" signal on Tabs/Name Tab menu item.
 * Put up a dialog to get a user specified name for the tab. */
static void terminal_name_tab_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint current = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current == -1 || terminal->terms->len <= 0) {
        return;
    }

    /* Search for the Term structure corresponding to the current tab. */
    Term *term = NULL;
    guint i;
    for (i = 0; i < terminal->terms->len; i++)
    {
        term = g_ptr_array_index(terminal->terms, i);
        if (term && term->index == current)
        {
            break;
        }
    }

    if (!term) return;

    GtkWidget *dialog = gtk_dialog_new_with_buttons(
        _("Name Tab"),
        GTK_WINDOW(terminal->window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        _("_Cancel"), GTK_RESPONSE_CANCEL,
        _("_OK"), GTK_RESPONSE_OK,
        NULL);
        
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
    
    if (gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "lxterminal"))
    {
        gtk_window_set_icon_name(GTK_WINDOW(dialog), "lxterminal");
    }
    else
    {
        gtk_window_set_icon_from_file(GTK_WINDOW(dialog), PACKAGE_DATA_DIR "/icons/hicolor/128x128/apps/lxterminal.png", NULL);
    }
    
    g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(terminal_name_tab_response_event), term);
    
    GtkWidget *dialog_item = gtk_entry_new();
    g_object_set_data(G_OBJECT(dialog), "entry", (gpointer)dialog_item);
    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), dialog_item, FALSE, FALSE, 2);
    
    if (term->label) {
        gtk_entry_set_text(GTK_ENTRY(dialog_item), gtk_label_get_text(GTK_LABEL(term->label)));
    }
    gtk_entry_set_activates_default(GTK_ENTRY(dialog_item), TRUE);
    gtk_widget_show_all(dialog);
}

/* Handler for "activate" signal on Tabs/Previous Tab menu item.
 * Cycle through tabs in the reverse direction. */
static void terminal_previous_tab_activate_event(GtkAction *action, LXTerminal *terminal)
{
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook)) == 0)
    {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal->notebook), -1);
    }
    else
    {
        gtk_notebook_prev_page(GTK_NOTEBOOK(terminal->notebook));
    }
}

/* Handler for "activate" signal on Tabs/Next Tab menu item.
 * Cycle through tabs in the forward direction. */
static void terminal_next_tab_activate_event(GtkAction *action, LXTerminal *terminal)
{
    gint max_pages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal->notebook));
    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook)) == (max_pages - 1))
    {
        gtk_notebook_set_current_page(GTK_NOTEBOOK(terminal->notebook), 0);
    }
    else
    {
        gtk_notebook_next_page(GTK_NOTEBOOK(terminal->notebook));
    }
}

/* Helper for move tab left and right. */
static void terminal_move_tab_execute(LXTerminal *terminal, gint direction)
{
    GtkNotebook *notebook = GTK_NOTEBOOK(terminal->notebook);
    gint current_page_number = gtk_notebook_get_current_page(notebook);
    gint target_page_number = current_page_number + direction;

    if (current_page_number < 0 || target_page_number < 0 || (guint)target_page_number >= terminal->terms->len)
    {
        return;
    }

    /* Swap index bindings inside terms array metadata pointer lists */
    Term *term_current = g_ptr_array_index(terminal->terms, current_page_number);
    Term *term_target = g_ptr_array_index(terminal->terms, target_page_number);
    
    if (term_current && term_target) {
        g_ptr_array_index(terminal->terms, target_page_number) = term_current;
        g_ptr_array_index(terminal->terms, current_page_number) = term_target;
        term_current->index = target_page_number;
        term_target->index = current_page_number;

        gtk_notebook_reorder_child(notebook, gtk_notebook_get_nth_page(notebook, current_page_number), target_page_number);

        /* Update alt sequence mappings accelerator list */
        terminal_update_alt(terminal);
    }
}

/* Handler for "activate" signal on Tabs/Move Tab Left menu item. */
static void terminal_move_tab_left_activate_event(GtkAction *action, LXTerminal *terminal)
{
    terminal_move_tab_execute(terminal, -1);
}

/* Handler for "activate" signal on Tabs/Move Tab Right menu item. */
static void terminal_move_tab_right_activate_event(GtkAction *action, LXTerminal *terminal)
{
    terminal_move_tab_execute(terminal, 1);
}

#if !(VTE_CHECK_VERSION (0, 38, 0))
/* Legacy uniform font scaling layer cross-backport for old structural configurations */
static void vte_terminal_set_font_scale(VteTerminal *vte, gdouble scale)
{
    Setting *setting = get_setting();
    if (!setting || !setting->font_name) return;
    
    PangoFontDescription *font_desc = pango_font_description_from_string(setting->font_name);
    if (font_desc) {
        gdouble current_size = pango_units_to_double(pango_font_description_get_size(font_desc));
        PangoFontDescription *new_font_desc = pango_font_description_copy(font_desc);
        pango_font_description_set_size(new_font_desc, pango_units_from_double(current_size * scale));
        vte_terminal_set_font(vte, new_font_desc);
        pango_font_description_free(new_font_desc);
    }
}
#endif

/* Synchronizes the layout proportions across zoom adjustments */
static void terminal_zoom(LXTerminal *terminal)
{
    Term *term = NULL;
    guint i;
    
    terminal_save_size(terminal);
    if (terminal->terms->len <= 0) {
        return;
    }

    for (i = 0; i < terminal->terms->len; i++) {
        term = g_ptr_array_index(terminal->terms, i);
        if (term) {
            vte_terminal_set_font_scale(VTE_TERMINAL(term->vte), terminal->scale);
        }
    }

    /* Grab the geometry hints profile metrics from the final processed valid entry */
    term = g_ptr_array_index(terminal->terms, 0);
    if (term) {
        GdkGeometry geometry = {0};
        terminal_set_geometry_hints(term, &geometry);

        gtk_window_resize(GTK_WINDOW(terminal->window),
                          geometry.base_width + geometry.width_inc * terminal->col,
                          geometry.base_height + geometry.height_inc * terminal->row);
    }
}

/* Handler for "activate" signal on Tabs/Zoom In */
static gboolean terminal_zoom_in_activate_event(GtkAction *action, LXTerminal *terminal)
{
    terminal->scale += 0.1;
    terminal_zoom(terminal);
    return FALSE;
}

/* Handler for "activate" signal on Tabs/Zoom Out */
static gboolean terminal_zoom_out_activate_event(GtkAction *action, LXTerminal *terminal)
{
    if (terminal->scale < 0.3)
        return FALSE;
    terminal->scale -= 0.1;
    terminal_zoom(terminal);
    return FALSE;
}

/* Handler for "activate" signal on Tabs/Zoom Reset */
static gboolean terminal_zoom_reset_activate_event(GtkAction *action, LXTerminal *terminal)
{
    terminal->scale = 1.0;
    terminal_zoom(terminal);
    return FALSE;
}

/* Handler for "activate" signal on Help/About menu item. */
static void terminal_about_activate_event(GtkAction *action, LXTerminal *terminal)
{
    const gchar *authors[] =
    {
        "Fred Chien <cfsghost@gmail.com>",
        "Marty Jack <martyj19@comcast.net>",
        "Yao Wei <mwei@lxde.org>",
        "Jonathan Thibault <jonathan@navigue.com>",
        NULL
    };
    gchar *translators = _("translator-credits");

    GtkWidget *about_dlg = gtk_about_dialog_new();
    gtk_container_set_border_width(GTK_CONTAINER(about_dlg), 2);
    gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about_dlg), VERSION);
    gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about_dlg), _("LXTerminal"));
    
    GdkPixbuf *logo = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR "/icons/hicolor/128x128/apps/lxterminal.png", NULL);
    if (logo) {
        gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about_dlg), logo);
        g_object_unref(logo);
    }
    
    gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about_dlg), _("Copyright (C) 2008-2026"));
    gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about_dlg), _("Terminal emulator for LXDE project"));
    gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about_dlg), 
        "This program is free software; you can redistribute it and/or\n"
        "modify it under the terms of the GNU General Public License\n"
        "as published by the Free Software Foundation; either version 2\n"
        "of the License, or (at your option) any later version.\n\n"
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        "GNU General Public License for more details.");
        
    gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about_dlg), "http://lxde.org/");
    gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about_dlg), authors);
    gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(about_dlg), translators);

    gtk_dialog_run(GTK_DIALOG(about_dlg));
    gtk_widget_destroy(about_dlg);
}

/* Handler for "switch-page" event on the tab notebook layout framework. */
static void terminal_switch_page_event(GtkNotebook *notebook, GtkWidget *page, guint num, LXTerminal *terminal)
{
    if (terminal->terms->len > num)
    {
        Term *term = g_ptr_array_index(terminal->terms, num);
        if (!term) return;

        const gchar *titaux;
        if (term->user_specified_label)
            titaux = gtk_label_get_text(GTK_LABEL(term->label));
        else
            titaux = vte_terminal_get_window_title(VTE_TERMINAL(term->vte));
            
        if (titaux && g_str_has_prefix(titaux, "* "))
            titaux += 2;
            
        gtk_label_set_markup(GTK_LABEL(term->label), titaux ? titaux : "");

        const gchar *title = gtk_label_get_text(GTK_LABEL(term->label));
        gtk_window_set_title(GTK_WINDOW(terminal->window), ((title != NULL) ? title : _("LXTerminal")));

        g_signal_connect(notebook, "size-allocate", G_CALLBACK(terminal_vte_size_allocate_event), term);
    }
}

/* Handler for "window-title-changed" signal on a Term layout wrapper. */
static void terminal_window_title_changed_event(GtkWidget *vte, Term *term)
{
    if (!term->user_specified_label)
    {
        const gchar *vte_title = vte_terminal_get_window_title(VTE_TERMINAL(vte));
        gtk_label_set_text(GTK_LABEL(term->label), vte_title ? vte_title : "");
        gtk_widget_set_tooltip_text(term->label, vte_title ? vte_title : "");
        
        GtkNotebook *notebook = GTK_NOTEBOOK(term->parent->notebook);
        gint current_page = gtk_notebook_get_current_page(notebook);
        if (current_page >= 0 && GTK_WIDGET(term->box) == gtk_notebook_get_nth_page(notebook, current_page)) {
            gtk_window_set_title(GTK_WINDOW(term->parent->window), vte_title ? vte_title : _("LXTerminal"));
        }
    }
}

/* Handler for "delete-event" signal on a LXTerminal window wrapper descriptor. */
static gboolean terminal_close_window_confirmation_event(GtkWidget *widget, GdkEventButton *event, LXTerminal *terminal)
{
    return !terminal_close_window_confirmation_dialog(terminal);
}

static gboolean terminal_close_window_confirmation_dialog(LXTerminal *terminal)
{
    if (!get_setting()->disable_confirm && terminal->terms->len > 1) {
        GtkWidget *dialog = gtk_message_dialog_new(
                GTK_WINDOW(terminal->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                _("You are about to close %d tabs. Are you sure you want to continue?"), terminal->terms->len);
        gtk_window_set_title(GTK_WINDOW(dialog), _("Confirm close"));
        gint result = gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        if (result != GTK_RESPONSE_YES) {
            return FALSE;
        }
    }
    return TRUE;
}

/* Weak-notify structural memory clean-up handler pipeline tracker block */
static void terminal_window_exit(LXTerminal *terminal, GObject *where_the_object_was)
{
    LXTermWindow *lxtermwin = terminal->parent;
    g_free(terminal->profile);

    if (lxtermwin->windows->len == 1) {
        gtk_main_quit();
    }
    else
    {
        g_ptr_array_remove_index(lxtermwin->windows, terminal->index);
        guint i;
        for (i = terminal->index; i < lxtermwin->windows->len; i++)
        {
            LXTerminal *t = g_ptr_array_index(lxtermwin->windows, i);
            if (t) t->index--;
        }
        g_slice_free(LXTerminal, terminal);
    }
}

/* Handler for "child-exited" signal on VTE backend hooks. */
#if VTE_CHECK_VERSION (0, 38, 0)
static void terminal_child_exited_event(VteTerminal *vte, gint status, Term *term)
#else
static void terminal_child_exited_event(VteTerminal *vte, Term *term)
#endif
{
    LXTerminal *terminal = term->parent;
    g_signal_handler_disconnect(G_OBJECT(term->vte), term->exit_handler_id);

    /* Last tab being deleted. Close the core window environment shell. */
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal->notebook)) == 1)
    {
        g_ptr_array_free(terminal->terms, TRUE);
        terminal->terms = g_ptr_array_new();
        gtk_widget_destroy(terminal->window);
    }
    else
    {
        guint target_idx = term->index;
        
        /* Remove the element and decrease the index tracking references sequentially */
        g_ptr_array_remove_index(terminal->terms, target_idx);
        guint i;
        for (i = target_idx; i < terminal->terms->len; i++)
        {
            Term *t = g_ptr_array_index(terminal->terms, i);
            if (t) t->index--;
        }

        /* Detach structural layouts */
        gtk_notebook_remove_page(GTK_NOTEBOOK(terminal->notebook), (gint)target_idx);
        terminal_free(term);

        if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal->notebook)) == 1) {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(terminal->notebook), FALSE);
        }

        terminal_update_alt(terminal);
    }
}

static void terminal_close_button_event(GtkButton *button, Term *term)
{
#if VTE_CHECK_VERSION (0, 38, 0)
    terminal_child_exited_event(VTE_TERMINAL(term->vte), 0, term);
#else
    terminal_child_exited_event(VTE_TERMINAL(term->vte), term);
#endif
}

static gboolean terminal_tab_button_press_event(GtkWidget *widget, GdkEventButton *event, Term *term)
{
    if (event->button == 2)
    {
        terminal_close_button_event(NULL, term);
        return TRUE;
    }
    return FALSE;
}

static gchar *terminal_get_match_at(VteTerminal *vte, Term *term, GdkEventButton *event)
{
#if VTE_CHECK_VERSION (0, 38, 0)
    gint tag;
    return vte_terminal_match_check_event(vte, (GdkEvent *)event, &tag);
#else
    GtkBorder border;
    terminal_get_border(term, &border);
    gint tag;
    return vte_terminal_match_check(vte,
        (event->x - border.left) / vte_terminal_get_char_width(vte),
        (event->y - border.top) / vte_terminal_get_char_height(vte),
        &tag);
#endif
}

static void terminal_show_popup_menu(VteTerminal *vte, GdkEventButton *event, Term *term)
{
    GtkUIManager *manager = gtk_ui_manager_new();
    GtkActionGroup *action_group = gtk_action_group_new("VTEMenu");
    gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);
    gtk_action_group_add_actions(action_group, vte_menu_items, VTE_MENUITEM_COUNT, term->parent);
    gtk_ui_manager_insert_action_group(manager, action_group, 0);

    guint merge_id = gtk_ui_manager_new_merge_id(manager);
    gtk_ui_manager_add_ui(manager, merge_id, "/", "VTEMenu", NULL, GTK_UI_MANAGER_POPUP, FALSE);
}
  size_t i;
    for (i = 1; i < VTE_MENUITEM_COUNT; i++)
    {
        if (strcmp(vte_menu_items[i].label, "Sep") == 0)
        {
            gtk_ui_manager_add_ui(manager, merge_id, "/VTEMenu",
                vte_menu_items[i].name, NULL, GTK_UI_MANAGER_SEPARATOR, FALSE);
        }
        else
        {
            gtk_ui_manager_add_ui(manager, merge_id, "/VTEMenu",
                vte_menu_items[i].name, vte_menu_items[i].name, GTK_UI_MANAGER_MENUITEM, FALSE);
        }
    }

    g_free(term->matched_url);
    term->matched_url = terminal_get_match_at(vte, term, event);

    GtkAction *action_copy_url = gtk_ui_manager_get_action(manager, "/VTEMenu/CopyURL");
    GtkAction *action_open_url = gtk_ui_manager_get_action(manager, "/VTEMenu/OpenURL");
    if (action_copy_url && action_open_url) {
        gtk_action_set_visible(action_copy_url, term->matched_url != NULL);
        gtk_action_set_visible(action_open_url, term->matched_url != NULL);
    }

#if GTK_CHECK_VERSION(3, 22, 0)
    gtk_menu_popup_at_pointer(GTK_MENU(gtk_ui_manager_get_widget(manager, "/VTEMenu")), (GdkEvent *)event);
#else
    gtk_menu_popup(GTK_MENU(gtk_ui_manager_get_widget(manager, "/VTEMenu")),
        NULL, NULL, NULL, NULL, event->button, event->time);
#endif
}

/* Handler for "cursor-moved" signal on VTE */
static void terminal_vte_cursor_moved_event(VteTerminal *vte, Term *term)
{
    LXTerminal *terminal = term->parent;
    gint current_page = gtk_notebook_get_current_page(GTK_NOTEBOOK(terminal->notebook));
    if (current_page < 0 || (guint)current_page >= terminal->terms->len) {
        return;
    }

    Term *activeterm = g_ptr_array_index(terminal->terms, current_page);
    /* If the activity occurs inside a background tab, set its label text layer to bold */
    if (activeterm != term) {
        if (term->user_specified_label) {
            const gchar *currentcustomtitle = gtk_label_get_text(GTK_LABEL(term->label));
            gchar *marked_title = NULL;

            /* Only prepend an asterisk if there isn't one already */
            if (!g_str_has_prefix(currentcustomtitle, "* ")) {
                gchar *temp_concat = g_strconcat("* ", currentcustomtitle, NULL);
                marked_title = g_strconcat("<b>", temp_concat, "</b>", NULL);
                g_free(temp_concat);
            } else {
                marked_title = g_strconcat("<b>", currentcustomtitle, "</b>", NULL);
            }

            gtk_label_set_markup(GTK_LABEL(term->label), marked_title);
            g_free(marked_title);
        } else {
            const gchar *vte_title = vte_terminal_get_window_title(VTE_TERMINAL(vte));
            gchar *marked_title = g_strconcat("<b>* ", vte_title ? vte_title : "", "</b>", NULL);
            gtk_label_set_markup(GTK_LABEL(term->label), marked_title);
            g_free(marked_title);
        }
    }
}

/* Handler for "bell" signal on VTE. Only connected when visual_bell is enabled. */
static void terminal_vte_visual_bell(VteTerminal *vte, Term *term)
{
    /* Toggle urgency hint window flash triggers */
    gtk_window_set_urgency_hint(GTK_WINDOW(term->parent->window), FALSE);
    gtk_window_set_urgency_hint(GTK_WINDOW(term->parent->window), TRUE);
}

/* Handler for "button-press-event" signal on VTE. */
static gboolean terminal_vte_button_press_event(VteTerminal *vte, GdkEventButton *event, Term *term)
{
    if (event->type != GDK_BUTTON_PRESS) /* Skip double/triple clicks */
        return FALSE;

    /* Right-click handling */
    if (event->button == 3)
    {
        if (event->state & GDK_CONTROL_MASK)
        {
            terminal_show_popup_menu(vte, event, term);
            return TRUE;
        }
        else
        {
            term->open_menu_on_button_release = TRUE;
        }
    }
    /* Control + Left Click URL activation pass */
    else if ((event->button == 1) && (event->state & GDK_CONTROL_MASK))
    {
        gchar *match = terminal_get_match_at(vte, term, event);
        if (match != NULL)
        {
            gchar *cmd = g_strdup_printf("xdg-open %s", match);
            if (!g_spawn_command_line_async(cmd, NULL)) {
                g_warning("Failed to launch xdg-open. The command was `%s'\n", cmd);
            }
            g_free(cmd);
            g_free(match);
        }
    }
    return FALSE;
}

static gboolean terminal_vte_button_release_event(VteTerminal *vte, GdkEventButton *event, Term *term)
{
    if (event->button == 3 && term->open_menu_on_button_release)
    {
        terminal_show_popup_menu(vte, event, term);
    }

    term->open_menu_on_button_release = FALSE;
    return FALSE;
}

static void terminal_vte_commit(VteTerminal *vte, gchar *text, guint size, Term *term)
{
    term->open_menu_on_button_release = FALSE;
}

/* Apply new settings in an LXTerminal to its tab Term. */
static void terminal_settings_apply_to_term(LXTerminal *terminal, Term *term)
{
    Setting *setting = get_setting();
    if (!setting) return;

#if VTE_CHECK_VERSION (0, 38, 0)
    PangoFontDescription *font_desc = pango_font_description_from_string(setting->font_name);
    if (font_desc) {
        vte_terminal_set_font(VTE_TERMINAL(term->vte), font_desc);
        pango_font_description_free(font_desc);
    }
    vte_terminal_set_word_char_exceptions(VTE_TERMINAL(term->vte), setting->word_selection_characters);
#else
    vte_terminal_set_font_from_string(VTE_TERMINAL(term->vte), setting->font_name);
    vte_terminal_set_word_chars(VTE_TERMINAL(term->vte), setting->word_selection_characters);
#endif

    vte_terminal_set_font_scale(VTE_TERMINAL(term->vte), terminal->scale);
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(term->vte), setting->scrollback);
    vte_terminal_set_allow_bold(VTE_TERMINAL(term->vte), !setting->disallow_bold);

#if VTE_CHECK_VERSION (0, 52, 0)
    vte_terminal_set_bold_is_bright(VTE_TERMINAL(term->vte), setting->bold_bright);
#endif

    vte_terminal_set_cursor_blink_mode(VTE_TERMINAL(term->vte), (setting->cursor_blink) ? VTE_CURSOR_BLINK_ON : VTE_CURSOR_BLINK_OFF);
    vte_terminal_set_cursor_shape(VTE_TERMINAL(term->vte), (setting->cursor_underline) ? VTE_CURSOR_SHAPE_UNDERLINE : VTE_CURSOR_SHAPE_BLOCK);
    vte_terminal_set_audible_bell(VTE_TERMINAL(term->vte), setting->audible_bell);
    vte_terminal_set_mouse_autohide(VTE_TERMINAL(term->vte), setting->hide_pointer);

#if !VTE_CHECK_VERSION (0, 38, 0)
    if (terminal->rgba)
    {
        vte_terminal_set_color_background(VTE_TERMINAL(term->vte), &setting->foreground_color);
        vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), FALSE);
        vte_terminal_set_opacity(VTE_TERMINAL(term->vte), setting->background_alpha);
    }
    else
    {
        vte_terminal_set_background_transparent(VTE_TERMINAL(term->vte), setting->background_alpha == 65535 ? FALSE : TRUE);
        vte_terminal_set_background_saturation(VTE_TERMINAL(term->vte), 1.0 - ((double)setting->background_alpha / 65535));
        vte_terminal_set_background_tint_color(VTE_TERMINAL(term->vte), &setting->background_color);
    }
#endif

#if VTE_CHECK_VERSION (0, 38, 0)
    const GdkRGBA *palette_color = setting->palette_color;
#else
    const GdkColor *palette_color = setting->palette_color;
#endif
    vte_terminal_set_colors(VTE_TERMINAL(term->vte), &setting->foreground_color, &setting->background_color, palette_color, 16);

    /* Hide or show scrollbar settings profiles */
    if (setting->hide_scroll_bar) {
        gtk_widget_hide(term->scrollbar);
    } else {
        gtk_widget_show(term->scrollbar);
    }

    /* Hide or show Close buttons */
    if (setting->hide_close_button) {
        gtk_widget_hide(term->close_button);
    } else {
        gtk_widget_show(term->close_button);
    }

    if (setting->visual_bell)
    {
        g_signal_connect(G_OBJECT(term->vte), "bell", G_CALLBACK(terminal_vte_visual_bell), term);
    }
    else
    {
        g_signal_handlers_disconnect_by_func(G_OBJECT(term->vte), terminal_vte_visual_bell, term);
    }
}

/* Create a new terminal backend node context map. */
static Term *terminal_new(LXTerminal *terminal, const gchar *label, const gchar *pwd, gchar **env, gchar **exec)
{
    Setting *setting = get_setting();

    Term *term = g_slice_new0(Term);
    term->parent = terminal;

    term->vte = vte_terminal_new();
#if GTK_CHECK_VERSION(3, 0, 0)
    term->box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    term->scrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
#else
    term->box = gtk_hbox_new(FALSE, 0);
    term->scrollbar = gtk_vscrollbar_new(NULL);
#endif
    gtk_box_pack_start(GTK_BOX(term->box), term->vte, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(term->box), term->scrollbar, FALSE, TRUE, 0);
    gtk_widget_set_no_show_all(GTK_WIDGET(term->scrollbar), TRUE);

#if GTK_CHECK_VERSION (2, 90, 8)
    GtkCssProvider *box_css_provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(box_css_provider, "box{background-color:@theme_bg_color;}", -1, NULL);
    GtkStyleContext *box_style_ctx = gtk_widget_get_style_context(GTK_WIDGET(terminal->box));
    gtk_style_context_add_provider(box_style_ctx, GTK_STYLE_PROVIDER(box_css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(box_css_provider);
#endif

    setlocale(LC_ALL, "");
#if VTE_CHECK_VERSION (0, 38, 0)
    vte_terminal_set_encoding(VTE_TERMINAL(term->vte), nl_langinfo(CODESET), NULL);
#else
    vte_terminal_set_emulation(VTE_TERMINAL(term->vte), "xterm");
    vte_terminal_set_encoding(VTE_TERMINAL(term->vte), nl_langinfo(CODESET));
#endif
    vte_terminal_set_backspace_binding(VTE_TERMINAL(term->vte), VTE_ERASE_ASCII_DELETE);
    vte_terminal_set_delete_binding(VTE_TERMINAL(term->vte), VTE_ERASE_DELETE_SEQUENCE);

#if VTE_CHECK_VERSION (0, 46, 0)
    VteRegex *dingus1 = vte_regex_new_for_match(DINGUS1, -1, PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_UCP | PCRE2_MULTILINE, NULL);
    VteRegex *dingus2 = vte_regex_new_for_match(DINGUS2, -1, PCRE2_UTF | PCRE2_NO_UTF_CHECK | PCRE2_UCP | PCRE2_MULTILINE, NULL);
    if (dingus1) {
        gint ret = vte_terminal_match_add_regex(VTE_TERMINAL(term->vte), dingus1, 0);
        vte_terminal_match_set_cursor_name(VTE_TERMINAL(term->vte), ret, "pointer");
        vte_regex_unref(dingus1);
    }
    if (dingus2) {
        gint ret = vte_terminal_match_add_regex(VTE_TERMINAL(term->vte), dingus2, 0);
        vte_terminal_match_set_cursor_name(VTE_TERMINAL(term->vte), ret, "pointer");
        vte_regex_unref(dingus2);
    }
#else
    GRegex *dingus1 = g_regex_new(DINGUS1, G_REGEX_OPTIMIZE, 0, NULL);
    GRegex *dingus2 = g_regex_new(DINGUS2, G_REGEX_OPTIMIZE, 0, NULL);
    if (dingus1) {
        gint ret = vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte), dingus1, 0);
        vte_terminal_match_set_cursor_name(VTE_TERMINAL(term->vte), ret, "pointer");
        g_regex_unref(dingus1);
    }
    if (dingus2) {
        gint ret = vte_terminal_match_add_gregex(VTE_TERMINAL(term->vte), dingus2, 0);
        vte_terminal_match_set_cursor_name(VTE_TERMINAL(term->vte), ret, "pointer");
        g_regex_unref(dingus2);
    }
#endif

    term->tab = gtk_event_box_new();
    gtk_widget_set_events(term->tab, GDK_BUTTON_PRESS_MASK);
#if GTK_CHECK_VERSION(3, 0, 0)
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
    GtkWidget *hbox = gtk_hbox_new(FALSE, 4);
#endif
    gtk_container_add(GTK_CONTAINER(term->tab), hbox);

    term->close_button = gtk_button_new();
    gtk_button_set_relief(GTK_BUTTON(term->close_button), GTK_RELIEF_NONE);
#if GTK_CHECK_VERSION (3, 20, 0)
    gtk_widget_set_focus_on_click(term->close_button, FALSE);
#else
    gtk_button_set_focus_on_click(GTK_BUTTON(term->close_button), FALSE);
#endif

#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_container_add(GTK_CONTAINER(term->close_button), gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU));
#else
    gtk_container_add(GTK_CONTAINER(term->close_button), gtk_image_new_from_stock(GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU));
#endif

#if !GTK_CHECK_VERSION(3, 0, 0)
    GtkRcStyle *rcstyle = gtk_rc_style_new();
    rcstyle->xthickness = rcstyle->ythickness = 0;
    gtk_widget_modify_style(term->close_button, rcstyle);
    g_object_unref(rcstyle);
#endif

    if (exec == NULL)
    {
        vte_terminal_feed(VTE_TERMINAL(term->vte), "\033]0;LXTerminal\007", -1);
    }
    else
    {
        gchar *cmd = g_path_get_basename((exec[1][0] == '-' && exec[2] != NULL) ? exec[3] : exec[1]);
        gchar *title = g_strdup_printf("\033]0;%s\007", cmd);
        vte_terminal_feed(VTE_TERMINAL(term->vte), title, -1);
        g_free(cmd);
        g_free(title);
    }

    /* Fixed single tracking allocation path for label element */
    if (label != NULL)
    {
        term->label = gtk_label_new(label);
        term->user_specified_label = TRUE;
    }
    else
    {
        term->label = gtk_label_new(vte_terminal_get_window_title(VTE_TERMINAL(term->vte)));
        term->user_specified_label = FALSE;
    }

    gtk_widget_set_size_request(GTK_WIDGET(term->label), setting->tab_width, -1);
    gtk_label_set_ellipsize(GTK_LABEL(term->label), PANGO_ELLIPSIZE_END);
#if GTK_CHECK_VERSION(3, 0, 0)
    gtk_widget_set_valign(term->label, GTK_ALIGN_CENTER);
#else
    gtk_misc_set_alignment(GTK_MISC(term->label), 0.0, 0.5);
    gtk_misc_set_padding(GTK_MISC(term->label), 0, 0);
#endif

    gtk_box_pack_start(GTK_BOX(hbox), term->label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), term->close_button, FALSE, FALSE, 0);
    gtk_widget_show_all(term->tab);

#if VTE_CHECK_VERSION (0, 38, 0)
    gtk_range_set_adjustment(GTK_RANGE(term->scrollbar), gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(term->vte)));
#else
    gtk_range_set_adjustment(GTK_RANGE(term->scrollbar), vte_terminal_get_adjustment(VTE_TERMINAL(term->vte)));
#endif

    if (exec == NULL)
    {
        exec = g_malloc(3 * sizeof(gchar *));
        exec[0] = g_strdup(terminal_get_preferred_shell());
        exec[1] = g_path_get_basename(exec[0]);
        exec[2] = NULL;
    }

#if VTE_CHECK_VERSION (0, 38, 0)
    vte_terminal_spawn_sync(
                    VTE_TERMINAL(term->vte),
                    VTE_PTY_NO_LASTLOG | VTE_PTY_NO_UTMP | VTE_PTY_NO_WTMP,
                    pwd,
                    exec,
                    env,
                    G_SPAWN_SEARCH_PATH | G_SPAWN_FILE_AND_ARGV_ZERO,
                    NULL, NULL,
                    &term->pid,
                    NULL, NULL);
#else
    vte_terminal_fork_command_full(
                    VTE_TERMINAL(term->vte),
                    VTE_PTY_NO_LASTLOG | VTE_PTY_NO_UTMP | VTE_PTY_NO_WTMP,
                    pwd,
                    exec,
                    env,
                    G_SPAWN_SEARCH_PATH | G_SPAWN_FILE_AND_ARGV_ZERO,
                    NULL, NULL,
                    &term->pid,
                    NULL);
#endif
    g_strfreev(exec);

    g_signal_connect(G_OBJECT(term->tab), "button-press-event", G_CALLBACK(terminal_tab_button_press_event), term);
    g_signal_connect(G_OBJECT(term->close_button), "clicked", G_CALLBACK(terminal_close_button_event), term);
    g_signal_connect(G_OBJECT(term->vte), "button-press-event", G_CALLBACK(terminal_vte_button_press_event), term);
    g_signal_connect(G_OBJECT(term->vte), "button-release-event", G_CALLBACK(terminal_vte_button_release_event), term);
    g_signal_connect(G_OBJECT(term->vte), "commit", G_CALLBACK(terminal_vte_commit), term);
    term->exit_handler_id = g_signal_connect(G_OBJECT(term->vte), "child-exited", G_CALLBACK(terminal_child_exited_event), term);
    g_signal_connect(G_OBJECT(term->vte), "cursor-moved", G_CALLBACK(terminal_vte_cursor_moved_event), term);
    g_signal_connect(G_OBJECT(term->vte), "window-title-changed", G_CALLBACK(terminal_window_title_changed_event), term);

    gtk_widget_show_all(term->box);
    terminal_settings_apply_to_term(terminal, term);
    
    return term;
}

/* Deallocate a Term structure. */
static void terminal_free(Term *term)
{
    if (!term) return;
    g_free(term->matched_url);
    if ((GTK_IS_ACCEL_GROUP(term->parent->accel_group)) && (term->closure != NULL))
    {
        gtk_accel_group_disconnect(term->parent->accel_group, term->closure);
    }
    g_slice_free(Term, term);
}

/* Initialize the menu bar structure elements. */
static void terminal_menubar_initialize(LXTerminal *terminal)
{
    GtkUIManager *manager = gtk_ui_manager_new();
    terminal->action_group = gtk_action_group_new("MenuBar");
    gtk_action_group_set_translation_domain(terminal->action_group, GETTEXT_PACKAGE);
    
    terminal_initialize_menu_shortcuts(get_setting());
    gtk_action_group_add_actions(terminal->action_group, menu_items, MENUBAR_MENUITEM_COUNT, terminal);
    gtk_ui_manager_insert_action_group(manager, terminal->action_group, 0);
    
    gtk_ui_manager_add_ui_from_file(manager, PACKAGE_DATA_DIR "/lxterminal/menu.ui", NULL);
    terminal->menu = gtk_ui_manager_get_widget(manager, "/MenuBar");
    
    terminal->accel_group = gtk_ui_manager_get_accel_group(manager);
    gtk_window_add_accel_group(GTK_WINDOW(terminal->window), terminal->accel_group);
}

/* Update the accelerator that brings up the window menu system bar. */
static void terminal_menu_accelerator_update(LXTerminal *terminal)
{
    if (saved_menu_accelerator == NULL)
    {
        g_object_get(G_OBJECT(gtk_settings_get_default()), "gtk-menu-bar-accel", &saved_menu_accelerator, NULL);
    }

#if GTK_CHECK_VERSION(3, 0, 0)
    g_object_set(
        gtk_settings_get_default(),
        "gtk-menu-bar-accel",
        ((get_setting()->disable_f10) ? "<Shift><Control><Mod1><Mod2><Mod3><Mod4><Mod5>F10" : saved_menu_accelerator),
        NULL);
#else
    gtk_settings_set_string_property(
        gtk_settings_get_default(),
        "gtk-menu-bar-accel",
        ((get_setting()->disable_f10) ? "<Shift><Control><Mod1><Mod2><Mod3><Mod4><Mod5>F10" : saved_menu_accelerator),
        "lxterminal");
#endif
}

/* Process the argument vector into the CommandArguments structure data layers. */
gboolean lxterminal_process_arguments(gint argc, gchar **argv, CommandArguments *arguments)
{
    memset(arguments, 0, sizeof(CommandArguments));
    arguments->executable = argv[0];

    gchar **argv_cursor = argv;
    gint cmd_len;

    while (argc > 1)
    {
        argc--;
        argv_cursor++;
        gchar *argument = *argv_cursor;

        if (strncmp(argument, "--command=", 10) == 0)
        {
            g_strfreev(arguments->command);
            g_shell_parse_argv(&argument[10], &cmd_len, &arguments->command, NULL);
        }
        else if ((strcmp(argument, "--command") == 0) || (strcmp(argument, "-e") == 0))
        {
            if (arguments->command != NULL) {
                g_strfreev(arguments->command);
            }
            cmd_len = 0;
            arguments->command = g_malloc(argc * sizeof(gchar *));

            while (argc > 1)
            {
                argc--;
                argv_cursor++;
                arguments->command[cmd_len] = g_strdup(*argv_cursor);
                cmd_len++;
            }
            arguments->command[cmd_len] = NULL;
        }
    }
    return TRUE;
}
        /* --geometry=<columns>x<rows> */
        else if (strncmp(argument, "--geometry=", 11) == 0)
        {
            int xoff, yoff;
            unsigned int width, height;
            const int bitmask = XParseGeometry(&argument[11], &xoff, &yoff, &width, &height);
            arguments->geometry_bitmask = bitmask;

            if (bitmask & WidthValue)
                arguments->geometry_columns = width;

            if (bitmask & HeightValue)
                arguments->geometry_rows = height;

            if (bitmask & XValue)
                arguments->geometry_xoff = xoff;

            if (bitmask & YValue)
                arguments->geometry_yoff = yoff;
        }

        /* -l, --loginshell */
        else if ((strcmp(argument, "--loginshell") == 0) || (strcmp(argument, "-l") == 0))
        {
            arguments->login_shell = TRUE;
        }

        /* --title=<title> */
        else if (strncmp(argument, "--title=", 8) == 0)
        {
            g_free(arguments->title);
            arguments->title = g_strdup(&argument[8]);
        }

        /* --tabs=<names> */
        else if (strncmp(argument, "--tabs=", 7) == 0)
        {
            g_free(arguments->tabs);
            arguments->tabs = g_strdup(&argument[7]);
        }

        /* -t <title>, -T <title>, --title <title> */
        else if (((strcmp(argument, "--title") == 0) || (strcmp(argument, "-t") == 0) || (strcmp(argument, "-T") == 0))
        && (argc > 1))
        {
            argc--;
            argv_cursor++;
            g_free(arguments->title);
            arguments->title = g_strdup(*argv_cursor);
        }

        /* --working-directory=<working directory> */
        else if (strncmp(argument, "--working-directory=", 20) == 0)
        {
            g_free(arguments->working_directory);
            arguments->working_directory = g_strdup(&argument[20]);
        }

        /* --profile=<config-identifier> */
        else if (strncmp(argument, "--profile=", 10) == 0)
        {
            g_free(arguments->profile);
            arguments->profile = g_strdup(&argument[10]);
        }

        /* --no-remote: Do not accept or send remote commands */
        else if (strcmp(argument, "--no-remote") == 0) {
            arguments->no_remote = TRUE;
        }

        /* -v or --version: Version information */
        else if (strcmp(argument, "-v") == 0 || strcmp(argument, "--version") == 0) {
            printf("%s\n", PACKAGE_STRING);
            return FALSE;
        }

        /* Undefined argument. Display help and exit */
        else {
            printf("%s\n", usage_display);
            return FALSE;
        }
    }

    /* Handle --loginshell context constraints */
    if (arguments->command != NULL && cmd_len <= 2) {
        /* Force using login shell if it has only 1 command, and command is not in PATH. */
        gchar *program_path = g_find_program_in_path(arguments->command[0]);
        if (program_path == NULL) {
            arguments->login_shell = TRUE;
        }
        g_free(program_path);
    }

    if (arguments->login_shell == TRUE)
    {
        const gchar *shell = terminal_get_preferred_shell();
        gchar *shellname = g_path_get_basename(shell);
        if (arguments->command == NULL)
        {
            arguments->command = g_malloc(3 * sizeof(gchar *));
            arguments->command[0] = g_strdup(shell);
            arguments->command[1] = g_strdup_printf("-%s", shellname);
            arguments->command[2] = NULL;
        }
        else
        {
            gchar **tmp = g_malloc((cmd_len + 4) * sizeof(gchar *));
            tmp[0] = g_strdup(shell);
            tmp[1] = g_strdup_printf("-%s", shellname);
            tmp[2] = g_strdup("-c");
            memcpy((tmp + 3), arguments->command, cmd_len * sizeof(gchar *));
            tmp[cmd_len + 3] = NULL;
            g_free(arguments->command);
            arguments->command = tmp;
        }
        g_free(shellname);
    }
    else
    {
        if (arguments->command != NULL)
        {
            gchar **tmp = g_malloc((cmd_len + 2) * sizeof(gchar *));
            tmp[0] = g_strdup(arguments->command[0]);
            memcpy((tmp + 1), arguments->command, cmd_len * sizeof(gchar *));
            tmp[cmd_len + 1] = NULL;
            g_free(arguments->command);
            arguments->command = tmp;
        }
    }
    return TRUE;
}

/* Initialize a new LXTerminal toplevel window environment. */
LXTerminal *lxterminal_initialize(LXTermWindow *lxtermwin, CommandArguments *arguments)
{
    LXTerminal *terminal = g_slice_new0(LXTerminal);
    terminal->parent = lxtermwin;
    terminal->terms = g_ptr_array_new();
    terminal->login_shell = arguments->login_shell;
    g_ptr_array_add(lxtermwin->windows, terminal);
    terminal->index = terminal->parent->windows->len - 1;
    terminal->scale = 1.0;
    terminal->profile = g_strdup(arguments->profile);
    Setting *setting = get_setting();

    terminal->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

#if GTK_CHECK_VERSION (2, 90, 8)
    GdkVisual *visual = gdk_screen_get_rgba_visual(gtk_widget_get_screen(GTK_WIDGET(terminal->window)));
    if (visual != NULL)
    {
        gtk_widget_set_visual(terminal->window, visual);
    }
#else
    GdkColormap *colormap = gdk_screen_get_rgba_colormap(gtk_widget_get_screen(GTK_WIDGET(terminal->window)));
    if (colormap != NULL)
    {
        gtk_widget_set_colormap(terminal->window, colormap);
    }
#endif

    if (gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), "lxterminal"))
    {
        gtk_window_set_icon_name(GTK_WINDOW(terminal->window), "lxterminal");
    }
    else
    {
        gtk_window_set_icon_from_file(GTK_WINDOW(terminal->window), 
            PACKAGE_DATA_DIR "/icons/hicolor/128x128/apps/lxterminal.png", NULL);
    }
    g_object_weak_ref(G_OBJECT(terminal->window), (GWeakNotify)terminal_window_exit, terminal);

#if GTK_CHECK_VERSION(3, 0, 0)
    terminal->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
#else
    terminal->box = gtk_vbox_new(FALSE, 1);
#endif
    gtk_container_add(GTK_CONTAINER(terminal->window), terminal->box);

    terminal_menubar_initialize(terminal);
    gtk_widget_set_no_show_all(GTK_WIDGET(terminal->menu), TRUE);
    if (setting->hide_menu_bar) {
        gtk_widget_hide(GTK_WIDGET(terminal->menu));
    }
    gtk_box_pack_start(GTK_BOX(terminal->box), terminal->menu, FALSE, TRUE, 0);

    terminal->notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(terminal->notebook), TRUE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(terminal->notebook), FALSE);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(terminal->notebook), FALSE);
    gtk_box_pack_start(GTK_BOX(terminal->box), terminal->notebook, TRUE, TRUE, 0);

    terminal->tab_position = terminal_tab_get_position_id(setting->tab_position);

    g_signal_connect_swapped(G_OBJECT(terminal->window), "composited-changed", 
        G_CALLBACK(terminal_settings_apply), terminal);
    g_signal_connect(G_OBJECT(terminal->notebook), "switch-page", 
        G_CALLBACK(terminal_switch_page_event), terminal);
    g_signal_connect(G_OBJECT(terminal->window), "delete-event",
        G_CALLBACK(terminal_close_window_confirmation_event), terminal);

    gchar *local_working_directory = NULL;
    if (arguments->working_directory == NULL)
    {
        local_working_directory = g_get_current_dir();
    }
    
    Term *term = terminal_new(
        terminal,
        ((arguments->title != NULL) ? arguments->title : NULL),
        ((arguments->working_directory != NULL) ? arguments->working_directory : local_working_directory),
        NULL,
        arguments->command);
    g_free(local_working_directory);

    if (term && term->label) {
        gtk_window_set_title(GTK_WINDOW(terminal->window), gtk_label_get_text(GTK_LABEL(term->label)));
    }

    const int geometry_bitmask = arguments->geometry_bitmask;

    if (term && term->vte) {
        if ((geometry_bitmask & WidthValue) && (geometry_bitmask & HeightValue)) {
            vte_terminal_set_size(VTE_TERMINAL(term->vte),
                                  arguments->geometry_columns,
                                  arguments->geometry_rows);
        } else {
            vte_terminal_set_size(VTE_TERMINAL(term->vte),
                                  setting->geometry_columns,
                                  setting->geometry_rows);
        }
    }

    if (term) {
        gtk_notebook_append_page(GTK_NOTEBOOK(terminal->notebook), term->box, term->tab);
        term->index = gtk_notebook_get_n_pages(GTK_NOTEBOOK(terminal->notebook)) - 1;
        g_ptr_array_add(terminal->terms, term);
    }

    gtk_widget_show_all(terminal->window);

    /* Post-render explicit application geometry placement calculations */
    if ((geometry_bitmask & XValue) && (geometry_bitmask & YValue)) {
        GtkWindow *const window = GTK_WINDOW(terminal->window);
        gint x, y;

        if (geometry_bitmask & XNegative) {
            GdkScreen *const screen = gtk_window_get_screen(window);
            gint window_width, window_height;
            gtk_window_get_size(window, &window_width, &window_height);
            x = gdk_screen_get_width(screen) - window_width + arguments->geometry_xoff;

            if (geometry_bitmask & YNegative) {
                y = gdk_screen_get_height(screen) - window_height + arguments->geometry_yoff;
                gtk_window_set_gravity(window, GDK_GRAVITY_SOUTH_EAST);
            } else {
                y = arguments->geometry_yoff;
                gtk_window_set_gravity(window, GDK_GRAVITY_NORTH_EAST);
            }
        } else {
            x = arguments->geometry_xoff;

            if (geometry_bitmask & YNegative) {
                gint window_width, window_height;
                gtk_window_get_size(window, &window_width, &window_height);
                y = gdk_screen_get_height(gtk_window_get_screen(window)) - window_height + arguments->geometry_yoff;
                gtk_window_set_gravity(window, GDK_GRAVITY_SOUTH_WEST);
            } else {
                y = arguments->geometry_yoff;
            }
        }
        gtk_window_move(window, x, y);
    }

    terminal_settings_apply(terminal);

    if (arguments->tabs != NULL && arguments->tabs[0] != '\0' && term)
    {
        char *token = strtok(arguments->tabs, ",");
        term->user_specified_label = TRUE;
        if (token) {
            gtk_label_set_text(GTK_LABEL(term->label), token);
            token = strtok(NULL, ",");
        }

        while (token != NULL && token[0] != '\0')
        {
            terminal_new_tab(terminal, token);
            token = strtok(NULL, ",");
        }
    }

    return terminal;
}

/* Apply new configuration rules directly to terminal layout properties. */
static void terminal_settings_apply(LXTerminal *terminal)
{
    Setting *setting = get_setting();
    if (!setting) return;

    terminal->rgba = gdk_screen_is_composited(gtk_widget_get_screen(terminal->window));

#if GTK_CHECK_VERSION (2, 90, 8) && VTE_CHECK_VERSION (0, 38, 0)
    gboolean has_transparency = setting->background_color.alpha < 1.0;
    gtk_widget_set_app_paintable(GTK_WIDGET(terminal->window), has_transparency);
#endif

    terminal->tab_position = terminal_tab_get_position_id(setting->tab_position);
    terminal_tab_set_position(terminal->notebook, terminal->tab_position);

    terminal_menu_accelerator_update(terminal);

    g_object_set(gtk_settings_get_default(), "gtk-enable-mnemonics", !setting->disable_alt, NULL);

    if (setting->hide_menu_bar) {
        gtk_widget_hide(terminal->menu);
    } else {
        gtk_widget_show(terminal->menu);
    }

    guint i;
    for (i = 0; i < terminal->terms->len; i++)
    {
        terminal_settings_apply_to_term(terminal, g_ptr_array_index(terminal->terms, i));
    }

    terminal_update_alt(terminal);

    GtkNotebook *notebook = GTK_NOTEBOOK(terminal->notebook);
    gint current_page_number = gtk_notebook_get_current_page(notebook);
    
    /* Boundary check added to guard access map lookups */
    if (current_page_number >= 0 && (guint)current_page_number < terminal->terms->len) {
        Term *term = g_ptr_array_index(terminal->terms, current_page_number);
        if (term && term->vte) {
            g_signal_connect(term->vte, "size-allocate", G_CALLBACK(terminal_vte_size_allocate_event), term);
        }
    }
}

/* Apply terminal configuration criteria across all open backend system maps. */
void terminal_settings_apply_to_all(LXTerminal *terminal)
{
    if (!terminal || !terminal->parent) return;
    terminal_update_menu_shortcuts(get_setting());
    g_ptr_array_foreach(terminal->parent->windows, (GFunc)terminal_settings_apply, get_setting());
    get_setting()->geometry_change = FALSE;
}

/* Update mapping properties for the main system accelerator layout engine. */
void terminal_update_menu_shortcuts(Setting *setting)
{
    if (!setting) return;
    guint key;
    GdkModifierType mods;

    gtk_accelerator_parse(setting->new_window_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/File_NewWindow", key, mods, FALSE);
    gtk_accelerator_parse(setting->new_tab_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/File_NewTab", key, mods, FALSE);
    gtk_accelerator_parse(setting->close_tab_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/File_CloseTab", key, mods, FALSE);
    gtk_accelerator_parse(setting->close_window_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/File_CloseWindow", key, mods, FALSE);
    gtk_accelerator_parse(setting->copy_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Edit_Copy", key, mods, FALSE);
    gtk_accelerator_parse(setting->paste_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Edit_Paste", key, mods, FALSE);
    gtk_accelerator_parse(setting->zoom_in_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Edit_ZoomIn", key, mods, FALSE);
    gtk_accelerator_parse(setting->zoom_out_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Edit_ZoomOut", key, mods, FALSE);
    gtk_accelerator_parse(setting->zoom_reset_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Edit_ZoomReset", key, mods, FALSE);
    gtk_accelerator_parse(setting->name_tab_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Tabs_NameTab", key, mods, FALSE);
    gtk_accelerator_parse(setting->previous_tab_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Tabs_PreviousTab", key, mods, FALSE);
    gtk_accelerator_parse(setting->next_tab_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Tabs_NextTab", key, mods, FALSE);
    gtk_accelerator_parse(setting->move_tab_left_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Tabs_MoveTabLeft", key, mods, FALSE);
    gtk_accelerator_parse(setting->move_tab_right_accel, &key, &mods);
    gtk_accel_map_change_entry("<Actions>/MenuBar/Tabs_MoveTabRight", key, mods, FALSE);
}

/* Initialize configuration shortcuts properties against target structures. */
void terminal_initialize_menu_shortcuts(Setting *setting)
{
    if (!setting) return;
    menu_items[4].accelerator = setting->new_window_accel;
    menu_items[5].accelerator = setting->new_tab_accel;
    menu_items[7].accelerator = setting->close_tab_accel;
    menu_items[8].accelerator = setting->close_window_accel;
    menu_items[9].accelerator = setting->copy_accel;
    menu_items[10].accelerator = setting->paste_accel;
    menu_items[13].accelerator = setting->zoom_in_accel;
    menu_items[14].accelerator = setting->zoom_out_accel;
    menu_items[15].accelerator = setting->zoom_reset_accel;
    menu_items[18].accelerator = setting->name_tab_accel;
    menu_items[19].accelerator = setting->previous_tab_accel;
    menu_items[20].accelerator = setting->next_tab_accel;
    menu_items[21].accelerator = setting->move_tab_left_accel;
    menu_items[22].accelerator = setting->move_tab_right_accel;
}

/* Main execution context entry point. */
int main(gint argc, gchar **argv)
{
    gtk_init(&argc, &argv);

#ifdef ENABLE_NLS
    bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    CommandArguments arguments;
    memset(&arguments, 0, sizeof(CommandArguments));
    if (!lxterminal_process_arguments(argc, argv, &arguments)) {
        g_free(arguments.title);
        g_free(arguments.tabs);
        g_free(arguments.working_directory);
        g_free(arguments.profile);
        return 0;
    }

    LXTermWindow *lxtermwin = g_slice_new0(LXTermWindow);

    if (!arguments.no_remote && !lxterminal_socket_initialize(lxtermwin, arguments.profile, argc, argv)) {
        g_free(arguments.title);
        g_free(arguments.tabs);
        g_free(arguments.working_directory);
        g_free(arguments.profile);
        g_slice_free(LXTermWindow, lxtermwin);
        return 0;
    }

    load_setting(arguments.profile);
    
    lxtermwin->windows = g_ptr_array_new();
    lxterminal_initialize(lxtermwin, &arguments);

    gtk_main();

    /* Structural post-loop memory teardown routines */
    g_free(arguments.title);
    g_free(arguments.tabs);
    g_free(arguments.working_directory);
    g_free(arguments.profile);

    return 0;
}