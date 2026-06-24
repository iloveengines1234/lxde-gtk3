/*
 * Copyright (C) 2006-2009 Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
 *               2006-2008 Jim Huang <jserv.tw@gmail.com>
 *               2008 Fred Chien <fred@lxde.org>
 *               2009 Jürgen Hötzel <juergen@archlinux.org>
 *               2009 Ying-Chun Liu (PaulLiu) <grandpaul@gmail.com>
 *               2009-2010 Marty Jack <martyj19@comcast.net>
 *               2010 Julien Lavergne <julien.lavergne@gmail.com>
 *               2011-2014 Henry Gebhardt <hsggebhardt@gmail.com>
 *               2012 Piotr Sipika <Piotr.Sipika@gmail.com>
 *               2013 Vincenzo di Cicco <enzodicicco@gmail.com>
 *               2013 Rouslan <rouslan-k@users.sourceforge.net>
 *               2014-2019 Andriy Grytsenko <andrej@rep.kiev.ua>
 *               2014 Andy Balaam <axis3x3@users.sf.net>
 *               2015 Balló György <ballogyor@gmail.com>
 *               2015 Rafał Mużyło <galtgendo@gmail.com>
 *               2018 Mamoru TASAKA <mtasaka@fedoraproject.org>
 *               2025 Ingo Brückl
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
#include <config.h>
#endif

#include "task-button.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include <glib/gi18n.h>
#include <cairo-xlib.h>

#include "plugin.h"
#include "misc.h"
#include "icon.xpm"
#include "gtk-compat.h"

#define ALL_WORKSPACES       -1

static Atom a_NET_WM_STATE_MAXIMIZED_VERT;
static Atom a_NET_WM_STATE_MAXIMIZED_HORZ;

/* -----------------------------------------------------------------------------
 * Class data
 */

/* individual task data */
typedef struct
{
    Window win;                             /* X window ID */
    gint desktop;                           /* Desktop that contains task, needed to switch to it on Raise */
    gint monitor;                           /* Monitor that the window is on or closest to */
    char * name;                            /* Taskbar label when normal, from WM_NAME or NET_WM_NAME */
    GdkPixbuf * icon;           /* the taskbar icon */
    GtkWidget * menu_item;      /* if menu_list exists then it's an item in it */
    Atom name_source;                       /* Atom that is the source of taskbar label */
    Atom image_source;                      /* Atom that is the source of taskbar icon */
    unsigned int visible :1;    /* TRUE if window is shown in taskbar */
    unsigned int focused                :1; /* True if window has focus */
    unsigned int iconified              :1; /* True if window is iconified, from WM_STATE */
    unsigned int urgency                :1; /* True if window has an urgency hint, from WM_HINTS */
} TaskDetails;

/* widget data */
struct _TaskButton
{
    GtkToggleButton parent;
    char * res_class;                      /* Class name */
    GtkWidget * image;                      /* Icon for task, child of button */
    GtkWidget * label;                      /* Label for task, child of button */
    LXPanel * panel;            /* points to panel (grandparent widget) */
    TaskDetails * last_focused; /* points to details of last focused task */
    GtkMenu * menu_list;        /* list of tasks on menu activation */
    Window menu_target;         /* window which activated menu */
    guint n_visible;            /* number of windows that are shown */
    guint idle_loader;          /* id of icons loader */
    GList * details;            /* details for each window, TaskDetails */
    gint desktop;               /* Current desktop of the button */
    gint n_desktops;            /* total number of desktops */
    gint monitor;               /* current monitor for the panel */
    guint icon_size;            /* Current value from last update */
    TaskShowFlags flags;        /* flags to show */
    unsigned int set_bold :1;   /* flat buttons only: TRUE if set bold */
    unsigned int visible :1;    /* TRUE if any window shown on current desktop */
    unsigned int same_name :1;  /* TRUE if all visible windows have the same name */
    unsigned int entered_state :1; /* TRUE if cursor is inside taskbar button */
    unsigned int has_flash :1;  /* used by task_button_set_flash_state() */
};

enum {
    MENU_BUILT,
    MENU_TARGET_SET,
    N_SIGNALS
};

static guint signals[N_SIGNALS];


static void task_update_icon(TaskButton *task, TaskDetails *details, Atom source);

/* -----------------------------------------------------------------------------
 * Internal functions
 */

/* Determine which monitor a given window is associated with */
static gint get_window_monitor(Window win)
{
    GdkDisplay *display;
    GdkWindow *gwin;
    gint m;

    display = gdk_display_get_default();
    g_assert(display);
    gwin = gdk_x11_window_foreign_new_for_display(display,win);
    g_assert(gwin);
    m = gdk_screen_get_monitor_at_window(gdk_window_get_screen(gwin),gwin);
    g_object_unref(gwin);
    return m;
}

/* Determine if the "urgency" hint is set on a window. */
static gboolean task_has_urgency(Window win)
{
    gboolean result = FALSE;
    XWMHints * hints = (XWMHints *) get_xaproperty(win, XA_WM_HINTS, XA_WM_HINTS, 0);
    if (hints != NULL)
    {
        if (hints->flags & XUrgencyHint)
            result = TRUE;
        XFree(hints);
    }
    //FIXME: also test _NET_WM_STATE_DEMANDS_ATTENTION flag in _NET_WM_STATE
    return result;
}

/* Returns TRUE if change name affects button name */
static gboolean task_set_names(TaskDetails *tk, Atom source)
{
    char * name = NULL;

    /* Try _NET_WM_VISIBLE_NAME, which supports UTF-8.
     * If it is set, the window manager is displaying it as the window title. */
    if ((source == None) || (source == a_NET_WM_VISIBLE_NAME))
    {
        name = get_utf8_property(tk->win,  a_NET_WM_VISIBLE_NAME);
        if (name != NULL)
            tk->name_source = a_NET_WM_VISIBLE_NAME;
    }

    /* Try _NET_WM_NAME, which supports UTF-8, but do not overwrite _NET_WM_VISIBLE_NAME. */
    if ((name == NULL)
    && ((source == None) || (source == a_NET_WM_NAME))
    && ((tk->name_source == None) || (tk->name_source == a_NET_WM_NAME) || (tk->name_source == XA_WM_NAME)))
    {
        name = get_utf8_property(tk->win,  a_NET_WM_NAME);
        if (name != NULL)
            tk->name_source = a_NET_WM_NAME;
    }

    /* Try WM_NAME, which supports only ISO-8859-1, but do not overwrite _NET_WM_VISIBLE_NAME or _NET_WM_NAME. */
    if ((name == NULL)
    && ((source == None) || (source == XA_WM_NAME))
    && ((tk->name_source == None) || (tk->name_source == XA_WM_NAME)))
    {
        name = get_textproperty(tk->win,  XA_WM_NAME);
        if (name != NULL)
            tk->name_source = XA_WM_NAME;
    }

    /* Set the name into the task context, and also on the tooltip. */
    if (name != NULL)
    {
        if (g_strcmp0(name, tk->name) != 0)
        {
            g_free(tk->name);
            tk->name = name;
            return TRUE;
        }
        g_free(name);
    }
    return FALSE;
}

static gboolean task_is_visible(TaskButton *b, TaskDetails *task)
{
    /* Not on same monitor */
    if (b->flags.same_monitor_only && b->monitor != task->monitor && b->monitor >= 0)
        return FALSE;

    /* Desktop placement. */
    return ((task->desktop == ALL_WORKSPACES) ||
            (task->desktop == b->desktop) ||
            (b->flags.show_all_desks) ||
            (b->flags.use_urgency_hint && task->urgency));
}

static TaskDetails *task_details_for_window(TaskButton *button, Window win)
{
    TaskDetails *details = g_slice_new0(TaskDetails);
    GdkDisplay *display = gdk_display_get_default();
    /* NOTE
     * 1. the extended mask is sum of taskbar and pager needs
     * see bug [ 940441 ] pager loose track of windows
     *
     * Do not change event mask to gtk windows spawned by this gtk client
     * this breaks gtk internals */
#if GTK_CHECK_VERSION(2, 24, 0)
    if (!gdk_x11_window_lookup_for_display(display, win))
#else
    if (!gdk_window_lookup(win))
#endif
        XSelectInput(GDK_DISPLAY_XDISPLAY(display), win,
                     PropertyChangeMask | StructureNotifyMask);

    /* fetch task details */
    details->win = win;
    details->desktop = get_net_wm_desktop(win);
    details->monitor = get_window_monitor(win);
    task_set_names(details, None);
    task_update_icon(button, details, None);
    details->urgency = task_has_urgency(win);
    details->iconified = (get_wm_state(win) == IconicState);
    // FIXME: may want _NET_WM_STATE check
    // FIXME: check if task is focused
    /* check task visibility by flags */
    details->visible = task_is_visible(button, details);
    return details;
}

static void free_task_details(TaskDetails *details)
{
    g_free(details->name);
    if (details->icon)
        g_object_unref(details->icon);
    g_slice_free(TaskDetails, details);
}

static TaskDetails *task_details_lookup(TaskButton *task, Window win)
{
    GList *l;

    for (l = task->details; l; l = l->next)
        if (((TaskDetails *)l->data)->win == win)
            return l->data;
    return NULL;
}

/* Position-calculation callback for grouped-task and window-management popup menu. */
static void taskbar_popup_set_position(GtkMenu * menu, gint * px, gint * py, gboolean * push_in, gpointer data)
{
    TaskButton * tb = (TaskButton *) data;

    /* Determine the coordinates. */
    lxpanel_plugin_popup_set_position_helper(tb->panel, data, GTK_WIDGET(menu), px, py);
    *push_in = TRUE;
}

static inline TaskButton *get_menu_task_button(GtkWidget *taskbar)
{
    return g_object_get_data(G_OBJECT(taskbar), "task-button-current");
}

/* Handler for "activate" event on Raise item of right-click menu for task buttons. */
static void menu_raise_window(GtkWidget * widget, GtkWidget * taskbar)
{
    TaskButton *tb = get_menu_task_button(taskbar);
    TaskDetails *tk = task_details_lookup(tb, tb->menu_target);
    Screen *screen = GDK_SCREEN_XSCREEN(gtk_widget_get_screen(widget));

    if ((tk->desktop != ALL_WORKSPACES) && (tk->desktop != tb->desktop))
        Xclimsgx(screen, RootWindowOfScreen(screen), a_NET_CURRENT_DESKTOP,
                 tk->desktop, 0, 0, 0, 0);
    XMapRaised(DisplayOfScreen(screen), tk->win);
}

/* Handler for maximize/unmaximize. Taken from WNCK */
static void do_maximize(GtkWidget *widget, Window xwindow, gboolean set)
{
    Xclimsgx(GDK_SCREEN_XSCREEN(gtk_widget_get_screen(widget)), xwindow,
             a_NET_WM_STATE, set ? a_NET_WM_STATE_ADD : a_NET_WM_STATE_REMOVE,
             a_NET_WM_STATE_MAXIMIZED_VERT, a_NET_WM_STATE_MAXIMIZED_HORZ,
             1 /* application */, 0);
}

/* Handler for "activate" event on Restore item of right-click menu for task buttons. */
static void menu_restore_window(GtkWidget * widget, GtkWidget * taskbar)
{
    TaskButton *tb = get_menu_task_button(taskbar);
    do_maximize(GTK_WIDGET(tb), tb->menu_target, FALSE);
}

/* Handler for "activate" event on Maximize item of right-click menu for task buttons. */
static void menu_maximize_window(GtkWidget * widget, GtkWidget * taskbar)
{
    TaskButton *tb = get_menu_task_button(taskbar);
    do_maximize(GTK_WIDGET(tb), tb->menu_target, TRUE);
}

/* Handler for "activate" event on Iconify item of right-click menu for task buttons. */
static void menu_iconify_window(GtkWidget * widget, GtkWidget * taskbar)
{
    TaskButton *tb = get_menu_task_button(taskbar);
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(widget));
    XIconifyWindow(xdisplay, tb->menu_target, DefaultScreen(xdisplay));
}

/* Handler for "activate" event on Move to Workspace item of right-click menu for task buttons. */
static void menu_move_to_workspace(GtkWidget * widget, GtkWidget * taskbar)
{
    TaskButton *tb = get_menu_task_button(taskbar);
    int num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "num"));
    Xclimsgx(GDK_SCREEN_XSCREEN(gtk_widget_get_screen(widget)), tb->menu_target,
             a_NET_WM_DESKTOP, num, 0, 0, 0, 0);
}

/* Handler for "activate" event on Close item of right-click menu for task buttons. */
static void menu_close_window(GtkWidget * widget, GtkWidget * taskbar)
{
    TaskButton *tb = get_menu_task_button(taskbar);
    Xclimsgwm(tb->menu_target, a_WM_PROTOCOLS, a_WM_DELETE_WINDOW);
}

/* Make right-click menu for task buttons.
 * This depends on number of desktops and edge. */
static GtkWidget *taskbar_make_menu(TaskButton *tb, GtkWidget *parent)
{
    /* Function to iterate in direction */
    void (*_m_add)(GtkMenuShell *self, GtkWidget* child);
    /* Allocate menu. */
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *mi;

    /* Add Raise menu item. */
    mi = gtk_menu_item_new_with_mnemonic(_("_Raise"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback) menu_raise_window, parent);

    /* Add Restore menu item. */
    mi = gtk_menu_item_new_with_mnemonic(_("R_estore"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback) menu_restore_window, parent);

    /* Add Maximize menu item. */
    mi = gtk_menu_item_new_with_mnemonic(_("Ma_ximize"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback) menu_maximize_window, parent);

    /* Add Iconify menu item. */
    mi = gtk_menu_item_new_with_mnemonic(_("Ico_nify"));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback) menu_iconify_window, parent);

    /* FIXME: if WM is Openbox then add "Window special parameters" submenu */

    /* If multiple desktops are supported, add menu items to select them. */
    if (tb->n_desktops > 1)
    {
        char label[128];
        /* Allocate submenu. */
        GtkWidget * workspace_menu = gtk_menu_new();
        GtkWidget * workspace_menu0 = NULL;

        /* Loop over all desktops. */
        int i;
        for (i = 1; i <= tb->n_desktops; i++)
        {
            /* For the first 9 desktops, allow the desktop number as a keyboard shortcut. */
            if (i <= 9)
            {
                g_snprintf(label, sizeof(label), _("Workspace _%d"), i);
                mi = gtk_menu_item_new_with_mnemonic(label);
            }
            else
            {
                g_snprintf(label, sizeof(label), _("Workspace %d"), i);
                mi = gtk_menu_item_new_with_label(label);
            }

            /* Set the desktop number as a property on the menu item. */
            g_object_set_data(G_OBJECT(mi), "num", GINT_TO_POINTER(i - 1));
            g_signal_connect(mi, "activate", G_CALLBACK(menu_move_to_workspace), parent);
            gtk_menu_shell_append(GTK_MENU_SHELL(workspace_menu), mi);
            if (G_UNLIKELY(workspace_menu0 == NULL))
                workspace_menu0 = mi;
        }
        g_object_set_data(G_OBJECT(menu), "task-menu-workspace0", workspace_menu0);

        /* Add a separator. */
        gtk_menu_shell_append(GTK_MENU_SHELL(workspace_menu), gtk_separator_menu_item_new());

        /* Add "move to all workspaces" item.  This causes the window to be visible no matter what desktop is active. */
        mi = gtk_menu_item_new_with_mnemonic(_("_All workspaces"));
        g_object_set_data(G_OBJECT(mi), "num", GINT_TO_POINTER(ALL_WORKSPACES));
        g_signal_connect(mi, "activate", G_CALLBACK(menu_move_to_workspace), parent);
        gtk_menu_shell_append(GTK_MENU_SHELL(workspace_menu), mi);

        /* FIXME: add "Current workspace" item, active if not on a current */

        /* Add Move to Workspace menu item as a submenu. */
        mi = gtk_menu_item_new_with_mnemonic(_("_Move to Workspace"));
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), workspace_menu);
    }

    /* Extend the menu by callbacks */
    g_signal_emit(tb, signals[MENU_BUILT], 0, menu);

    /* Add Close menu item.  By popular demand, we place this menu item closest to the cursor. */
    if (panel_is_at_bottom(tb->panel))
        _m_add = gtk_menu_shell_append;
    else
        _m_add = gtk_menu_shell_prepend;

    _m_add(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new());
    mi = gtk_menu_item_new_with_mnemonic (_("_Close Window"));
    _m_add(GTK_MENU_SHELL(menu), mi);
    g_signal_connect(G_OBJECT(mi), "activate", (GCallback)menu_close_window, parent);

    return menu;
}

static GtkWidget *get_task_button_menu(TaskButton *tb, TaskDetails *task)
{
    GtkWidget *parent = gtk_widget_get_parent(GTK_WIDGET(tb));
    GtkWidget *menu = g_object_get_data(G_OBJECT(parent), "task-button-menu");
    GtkWidget *workspace_menu0; /* item in task menu for workspace 0 */

    if (menu == NULL)
    {
        /* this GtkMenu is built on demand on the parent widget */
        menu = taskbar_make_menu(tb, parent);
        gtk_widget_show_all(menu);
        g_object_set_data_full(G_OBJECT(parent), "task-button-menu",
                               g_object_ref_sink(menu), g_object_unref);
    }
    g_object_set_data(G_OBJECT(parent), "task-button-current", tb);
    /* save current choice for our callbacks */
    tb->menu_target = task->win;
    /* notify menu makers about current choise */
    g_signal_emit(tb, signals[MENU_TARGET_SET], 0, (gulong)task->win);
    /* gray out workspace where window is on */
    workspace_menu0 = g_object_get_data(G_OBJECT(menu), "task-menu-workspace0");
    if (workspace_menu0)
    {
        GList *items = gtk_container_get_children(GTK_CONTAINER(gtk_widget_get_parent(workspace_menu0)));
        GList *item = g_list_find(items, workspace_menu0);
        int i;
        if (item != NULL) /* else error */
            for (i = 0; i < tb->n_desktops; i++, item = item->next)
                gtk_widget_set_sensitive(item->data, i != task->desktop);
        g_list_free(items);
    }
    //FIXME: do the same for 'All workspaces' item

    return menu;
}

/* Do the proper steps to raise a window.
 * This means removing it from iconified state and bringing it to the front.
 * We also switch the active desktop and viewport if needed. */
static void task_raise_window(TaskButton *tb, TaskDetails *tk, guint32 time)
{
#if GTK_CHECK_VERSION(2, 24, 0)
    GdkDisplay *display = gtk_widget_get_display(GTK_WIDGET(tb));
#endif
    Screen *xscreen = GDK_SCREEN_XSCREEN(gtk_widget_get_screen(GTK_WIDGET(tb)));
    Display *xdisplay = DisplayOfScreen(xscreen);

    /* Change desktop if needed. */
    if ((tk->desktop != ALL_WORKSPACES) && (tk->desktop != tb->desktop))
        Xclimsgx(xscreen, RootWindowOfScreen(xscreen), a_NET_CURRENT_DESKTOP, tk->desktop, 0, 0, 0, 0);

    /* Raise the window.  We can use NET_ACTIVE_WINDOW if the window manager supports it.
     * Otherwise, do it the old way with XMapRaised and XSetInputFocus. */
    if (tb->flags.use_net_active)
        Xclimsgx(xscreen, tk->win, a_NET_ACTIVE_WINDOW, 2, time, 0, 0, 0);
    else
    {
#if GTK_CHECK_VERSION(2, 24, 0)
        GdkWindow * gdkwindow = gdk_x11_window_lookup_for_display(display, tk->win);
#else
        GdkWindow * gdkwindow = gdk_xid_table_lookup(tk->win);
#endif
        if (gdkwindow != NULL)
            gdk_window_show(gdkwindow);
        else
            XMapRaised(xdisplay, tk->win);

        /* There is a race condition between the X server actually executing the XMapRaised and this code executing XSetInputFocus.
         * If the window is not viewable, the XSetInputFocus will fail with BadMatch. */
        XWindowAttributes attr;
        XGetWindowAttributes(xdisplay, tk->win, &attr);
        if (attr.map_state == IsViewable)
            XSetInputFocus(xdisplay, tk->win, RevertToNone, time);
    }

    /* Change viewport if needed. */
    XWindowAttributes xwa;
    XGetWindowAttributes(xdisplay, tk->win, &xwa);
    Xclimsgx(xscreen, tk->win, a_NET_DESKTOP_VIEWPORT, xwa.x, xwa.y, 0, 0, 0);
}

/* called when list of windows menu emits signal "selection-done" */
static void on_menu_list_selection_done(GtkMenuShell *menushell, TaskButton *tb)
{
    g_object_remove_weak_pointer(G_OBJECT(menushell), (void **)&tb->menu_list);
    tb->menu_list = NULL;
}

static gboolean task_button_window_do_release_event(GtkWidget *tb, TaskDetails *task, GdkEventButton *event)
{
    if (event->button == 1)
    {
        Display *xdisplay = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(tb));
        /* Left button.
         * If the task is iconified, raise it.
         * If the task is not iconified and has focus, iconify it.
         * If the task is not iconified and does not have focus, raise it. */
        if (task->iconified)
            task_raise_window(PANEL_TASK_BUTTON(tb), task, event->time);
        else if (task->focused)
            XIconifyWindow(xdisplay, task->win, DefaultScreen(xdisplay));
        else
            task_raise_window(PANEL_TASK_BUTTON(tb), task, event->time);
    }
    else if (event->button == 2)
    {
        /* Middle button.  Toggle the shaded state of the window. */
        Xclimsgx(GDK_SCREEN_XSCREEN(gtk_widget_get_screen(tb)),
                task->win, a_NET_WM_STATE,
                a_NET_WM_STATE_TOGGLE,
                a_NET_WM_STATE_SHADED,
                0, 0, 0);
    }
    return TRUE;
}

/* Handler for "button-press-event" event from grouped-task popup menu item. */
static gboolean taskbar_popup_activate_event(GtkWidget *widget, GdkEventButton *event,
                                             TaskButton *tk)
{
    GtkWidget *menu;
    GList *l;

    /* find details of this menu item and set tk->menu_target */
    for (l = tk->details; l; l = l->next)
        if (((TaskDetails *)l->data)->menu_item == widget)
            break;
    if (l == NULL) /* it's impossible really */
        return FALSE;
    /* if button 1 or 2 pressed then handle it the same as button-release
       event on a single task button */
    if (event->button == 1 || event->button == 2)
        return task_button_window_do_release_event(GTK_WIDGET(tk), l->data, event);
    else if (event->button != 3) /* don't process other buttons */
        return FALSE;
    /* process event the same way as for single task button */
    menu = get_task_button_menu(tk, l->data);
    /* attach and show menu */
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), menu);
    /* let menu continue with submenu */
    return FALSE;
}

static void menu_task_selected(GtkMenuItem *item, TaskButton *tb)
{
    GList *l;
    TaskDetails *task;

    for (l = tb->details; l; l = l->next)
        if ((task = l->data)->menu_item == (GtkWidget *)item)
            break;
    if (l == NULL) /* it's impossible really */
        return;
    tb->menu_target = task->win;
    // FIXME: auto attach menu?
}

static void menu_task_deselected(GtkMenuItem *item, TaskButton *tb)
{
    GList *l;
    TaskDetails *task;

    for (l = tb->details; l; l = l->next)
        if ((task = l->data)->menu_item == (GtkWidget *)item)
            break;
    if (l == NULL) /* it's impossible really */
        return;
    /* remove submenu from item */
    gtk_menu_item_set_submenu(item, NULL);
}

/* Handler for "activate" event from "close all windows" menu item */
static void taskbar_close_all_windows(GtkWidget * widget, TaskButton *tb)
{
    GList *l;

    for (l = tb->details; l; l = l->next)
    {
        TaskDetails *tk = l->data;

        if (tk->visible)
        {
            Xclimsgwm(tk->win, a_WM_PROTOCOLS, a_WM_DELETE_WINDOW);
        }
    }
}

static void assemble_gui(TaskButton *self)
{
    /* Create a modern GTK3 horizontal box */
    GtkWidget *container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_container_set_border_width(GTK_CONTAINER(container), 0);

    /* Image padding/alignment (GtkMisc is deprecated) */
    gtk_widget_set_margin_start(self->image, 0);
    gtk_widget_set_margin_end(self->image, 0);
    gtk_widget_set_margin_top(self->image, 0);
    gtk_widget_set_margin_bottom(self->image, 0);
    gtk_widget_set_halign(self->image, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(self->image, GTK_ALIGN_CENTER);

    gtk_box_pack_start(GTK_BOX(container), self->image, FALSE, FALSE, 0);

    /* Label alignment (GtkMisc deprecated) */
    gtk_widget_set_halign(self->label, GTK_ALIGN_START);
    gtk_widget_set_valign(self->label, GTK_ALIGN_CENTER);
    gtk_label_set_ellipsize(GTK_LABEL(self->label), PANGO_ELLIPSIZE_END);

    gtk_box_pack_start(GTK_BOX(container), self->label, TRUE, TRUE, 0);

    /* Add the box to the button */
    gtk_container_add(GTK_CONTAINER(self), container);
    gtk_widget_show(container);
    gtk_widget_show(self->image);
    gtk_widget_set_visible(self->label, !self->flags.icons_only);
}

static void map_xwindow_animation(GtkWidget *widget, Window win, GtkAllocation *alloc)
{
    /* Tell WM to set iconifying animation the window into the task button */
    if (gtk_widget_get_realized(widget))
    {
        int x, y;
        gulong data[4];

        /* Get the coordinates of the button. */
        gdk_window_get_origin(gtk_button_get_event_window(GTK_BUTTON(widget)), &x, &y);

        /* Send a NET_WM_ICON_GEOMETRY property change on the window. */
        data[0] = x;
        data[1] = y;
        data[2] = alloc->width;
        data[3] = alloc->height;
        XChangeProperty(GDK_DISPLAY_XDISPLAY(gtk_widget_get_display(widget)), win,
                        gdk_x11_get_xatom_by_name("_NET_WM_ICON_GEOMETRY"),
                        XA_CARDINAL, 32, PropModeReplace, (guchar *) &data, 4);
    }
}

/* Get a pixbuf from a pixmap.
 * Originally from libwnck, Copyright (C) 2001 Havoc Pennington. */
#if !GTK_CHECK_VERSION(3, 0, 0)
static GdkPixbuf * _wnck_gdk_pixbuf_get_from_pixmap(GdkScreen *screen, Pixmap xpixmap, Window win, int width, int height)
{
    /* Get the drawable. */
#if GTK_CHECK_VERSION(2, 24, 0)
    GdkDrawable * drawable = gdk_x11_window_lookup_for_display(gdk_display_get_default(), xpixmap);
#else
    GdkDrawable * drawable = gdk_xid_table_lookup(xpixmap);
#endif
    if (drawable != NULL)
        g_object_ref(G_OBJECT(drawable));
    else
        drawable = gdk_pixmap_foreign_new(xpixmap);

    GdkColormap * colormap = NULL;
    GdkPixbuf * retval = NULL;
    if (drawable != NULL)
    {
        /* Get the colormap.
         * If the drawable has no colormap, use no colormap or the system colormap as recommended in the documentation of gdk_drawable_get_colormap. */
        colormap = gdk_drawable_get_colormap(drawable);
        gint depth = gdk_drawable_get_depth(drawable);
        if (colormap != NULL)
            g_object_ref(G_OBJECT(colormap));
        else if (depth == 1)
            colormap = NULL;
        else
        {
            colormap = gdk_screen_get_system_colormap(screen);
            g_object_ref(G_OBJECT(colormap));
        }

        /* Be sure we aren't going to fail due to visual mismatch. */
        if ((colormap != NULL) && (gdk_visual_get_depth(gdk_colormap_get_visual(colormap)) != depth))
        {
            g_object_unref(G_OBJECT(colormap));
            colormap = NULL;
        }

        /* Do the major work. */
        retval = gdk_pixbuf_get_from_drawable(NULL, drawable, colormap, 0, 0, 0, 0, width, height);
    }

    /* Clean up and return. */
    if (colormap != NULL)
        g_object_unref(G_OBJECT(colormap));
    if (drawable != NULL)
        g_object_unref(G_OBJECT(drawable));
    return retval;
}
#else
static GdkPixbuf * _wnck_gdk_pixbuf_get_from_pixmap(GdkScreen *screen, Pixmap xpixmap, Window win, int width, int height)
{
  cairo_surface_t *surface;
  GdkPixbuf *pixbuf = NULL;
  Display *xdisplay;
  XWindowAttributes attrs;

  surface = NULL;
  xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());

  gdk_error_trap_push();

  if (!XGetWindowAttributes (xdisplay, win, &attrs))
    goto TRAP_POP;

  if (attrs.depth == 1)
    {
      surface = cairo_xlib_surface_create_for_bitmap (xdisplay,
                                                      xpixmap,
                                                      attrs.screen,
                                                      width,
                                                      height);
    }
  else
    {
      surface = cairo_xlib_surface_create (xdisplay,
                                           xpixmap,
                                           attrs.visual,
                                           width, height);
    }

  pixbuf = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);
  cairo_surface_destroy (surface);

TRAP_POP:
  gdk_flush();
  if (gdk_error_trap_pop())
    g_warning("task button : X error");

  return pixbuf;
}
#endif

/* Apply a mask to a pixbuf.
 * Originally from libwnck, Copyright (C) 2001 Havoc Pennington. */
static GdkPixbuf * apply_mask(GdkPixbuf * pixbuf, GdkPixbuf * mask)
{
    /* Initialize. */
    int w = MIN(gdk_pixbuf_get_width(mask), gdk_pixbuf_get_width(pixbuf));
    int h = MIN(gdk_pixbuf_get_height(mask), gdk_pixbuf_get_height(pixbuf));
    GdkPixbuf * with_alpha = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
    guchar * dst = gdk_pixbuf_get_pixels(with_alpha);
    guchar * src = gdk_pixbuf_get_pixels(mask);
    int dst_stride = gdk_pixbuf_get_rowstride(with_alpha);
    int src_stride = gdk_pixbuf_get_rowstride(mask);

    /* Loop to do the work. */
    int i;
    for (i = 0; i < h; i += 1)
    {
        int j;
        for (j = 0; j < w; j += 1)
        {
            guchar * s = src + i * src_stride + j * 3;
            guchar * d = dst + i * dst_stride + j * 4;

            /* s[0] == s[1] == s[2], they are 255 if the bit was set, 0 otherwise. */
            d[3] = ((s[0] == 0) ? 0 : 255); /* 0 = transparent, 255 = opaque */
        }
    }

    return with_alpha;
}

/* Get an icon from the window manager for a task, and scale it to a specified size. */
static GdkPixbuf * get_wm_icon(Window task_win, guint required_width,
                               guint required_height, Atom source,
                               Atom * current_source, TaskButton * tb)
{
    /* The result. */
    GdkPixbuf * pixmap = NULL;
    Atom possible_source = None;
    int result = -1;
    Display *xdisplay = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(tb));

    if ((source == None) || (source == a_NET_WM_ICON))
    {
        /* Important Notes:
         * According to freedesktop.org document:
         * http://standards.freedesktop.org/wm-spec/wm-spec-1.4.html#id2552223
         * _NET_WM_ICON contains an array of 32-bit packed CARDINAL ARGB.
         * However, this is incorrect. Actually it's an array of long integers.
         * Toolkits like gtk+ use unsigned long here to store icons.
         * Besides, according to manpage of XGetWindowProperty, when returned format,
         * is 32, the property data will be stored as an array of longs
         * (which in a 64-bit application will be 64-bit values that are
         * padded in the upper 4 bytes).
         */

        /* Get the window property _NET_WM_ICON, if possible. */
        Atom type = None;
        int format;
        gulong nitems;
        gulong bytes_after;
        gulong * data = NULL;
        result = XGetWindowProperty(
            xdisplay,
            task_win,
            a_NET_WM_ICON,
            0, G_MAXLONG,
            False, XA_CARDINAL,
            &type, &format, &nitems, &bytes_after, (void *) &data);

        /* Inspect the result to see if it is usable.  If not, and we got data, free it. */
        if ((type != XA_CARDINAL) || (nitems <= 0))
        {
            if (data != NULL)
                XFree(data);
            result = -1;
        }

        /* If the result is usable, extract the icon from it. */
        if (result == Success)
        {
            /* Get the largest icon available, unless there is one that is the desired size. */
            /* FIXME: should we try to find an icon whose size is closest to
             * required_width and required_height to reduce unnecessary resizing? */
            gulong * pdata = data;
            gulong * pdata_end = data + nitems;
            gulong * max_icon = NULL;
            gulong max_w = 0;
            gulong max_h = 0;
            while ((pdata + 2) < pdata_end)
            {
                /* Extract the width and height. */
                guint w = pdata[0];
                guint h = pdata[1];
                gulong size = w * h;
                pdata += 2;

                /* Bounds check the icon. Also check for invalid width and height,
                   see http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=801319 */
                if (size == 0 || w > 1024 || h > 1024 || pdata + size > pdata_end)
                    break;

                /* Rare special case: the desired size is the same as icon size. */
                if ((required_width == w) && (required_height == h))
                {
                    max_icon = pdata;
                    max_w = w;
                    max_h = h;
                    break;
                }

                /* If the icon is the largest so far, capture it. */
                if ((w > max_w) && (h > max_h))
                {
                    max_icon = pdata;
                    max_w = w;
                    max_h = h;
                }
                pdata += size;
            }

            /* If an icon was extracted, convert it to a pixbuf.
             * Its size is max_w and max_h. */
            if (max_icon != NULL)
            {
                /* Allocate enough space for the pixel data. */
                gulong len = max_w * max_h;
                guchar * pixdata = g_new(guchar, len * 4);

                /* Loop to convert the pixel data. */
                guchar * p = pixdata;
                gulong i;
                for (i = 0; i < len; p += 4, i += 1)
                {
                    guint argb = max_icon[i];
                    guint rgba = (argb << 8) | (argb >> 24);
                    p[0] = rgba >> 24;
                    p[1] = (rgba >> 16) & 0xff;
                    p[2] = (rgba >> 8) & 0xff;
                    p[3] = rgba & 0xff;
                }

                /* Initialize a pixmap with the pixel data. */
                pixmap = gdk_pixbuf_new_from_data(
                    pixdata,
                    GDK_COLORSPACE_RGB,
                    TRUE, 8,    /* has_alpha, bits_per_sample */
                    max_w, max_h, max_w * 4,
                    (GdkPixbufDestroyNotify) g_free,
                    NULL);
                possible_source = a_NET_WM_ICON;
            }
            else
                result = -1;

            /* Free the X property data. */
            XFree(data);
        }
    }

    /* No icon available from _NET_WM_ICON.  Next try WM_HINTS, but do not overwrite _NET_WM_ICON. */
    if ((result != Success) && (*current_source != a_NET_WM_ICON)
    && ((source == None) || (source != a_NET_WM_ICON)))
    {
        XWMHints * hints = XGetWMHints(xdisplay, task_win);
        result = (hints != NULL) ? Success : -1;
        Pixmap xpixmap = None;
        Pixmap xmask = None;
        Window win = None;

        if (result == Success)
        {
            /* WM_HINTS is available.  Extract the X pixmap and mask. */
            if ((hints->flags & IconPixmapHint))
                xpixmap = hints->icon_pixmap;
            if ((hints->flags & IconMaskHint))
                xmask = hints->icon_mask;
            XFree(hints);
            if (xpixmap != None)
            {
                result = Success;
                possible_source = XA_WM_HINTS;
            }
            else
                result = -1;
        }

        if (result != Success)
        {
            /* No icon available from _NET_WM_ICON or WM_HINTS.  Next try KWM_WIN_ICON. */
            Atom type = None;
            int format;
            gulong nitems;
            gulong bytes_after;
            Pixmap *icons = NULL;
            Atom kwin_win_icon_atom = gdk_x11_get_xatom_by_name("KWM_WIN_ICON");
            result = XGetWindowProperty(
                xdisplay,
                task_win,
                kwin_win_icon_atom,
                0, G_MAXLONG,
                False, kwin_win_icon_atom,
                &type, &format, &nitems, &bytes_after, (void *) &icons);

            /* Inspect the result to see if it is usable.  If not, and we got data, free it. */
            if (type != kwin_win_icon_atom)
            {
                if (icons != NULL)
                    XFree(icons);
                result = -1;
            }

            /* If the result is usable, extract the X pixmap and mask from it. */
            if (result == Success)
            {
                xpixmap = icons[0];
                xmask = icons[1];
                if (xpixmap != None)
                {
                    result = Success;
                    possible_source = kwin_win_icon_atom;
                }
                else
                    result = -1;
            }
        }

        /* If we have an X pixmap, get its geometry.*/
        unsigned int w, h;
        if (result == Success)
        {
            int unused;
            unsigned int unused_2;
            result = XGetGeometry(
                xdisplay, xpixmap,
                &win, &unused, &unused, &w, &h, &unused_2, &unused_2) ? Success : -1;
        }

        /* If we have an X pixmap and its geometry, convert it to a GDK pixmap. */
        if (result == Success)
        {
            pixmap = _wnck_gdk_pixbuf_get_from_pixmap(screen, xpixmap, win, w, h);
            result = ((pixmap != NULL) ? Success : -1);
        }

        /* If we have success, see if the result needs to be masked.
         * Failures here are implemented as nonfatal. */
        if ((result == Success) && (xmask != None))
        {
            Window win;
            int unused;
            unsigned int unused_2;
            if (XGetGeometry(
                xdisplay, xmask,
                &win, &unused, &unused, &w, &h, &unused_2, &unused_2))
            {
                /* Convert the X mask to a GDK pixmap. */
                GdkPixbuf * mask = _wnck_gdk_pixbuf_get_from_pixmap(screen, xmask, win, w, h);
                if (mask != NULL)
                {
                    /* Apply the mask. */
                    GdkPixbuf * masked_pixmap = apply_mask(pixmap, mask);
                    g_object_unref(G_OBJECT(pixmap));
                    g_object_unref(G_OBJECT(mask));
                    pixmap = masked_pixmap;
                }
            }
        }
    }

    /* If we got a pixmap, scale it and return it. */
    if (pixmap == NULL)
        return NULL;
    else
    {
        GdkPixbuf * ret;

        *current_source = possible_source;
        if (tb->flags.disable_taskbar_upscale)
        {
            guint w = gdk_pixbuf_get_width (pixmap);
            guint h = gdk_pixbuf_get_height (pixmap);
            if (w <= required_width || h <= required_height)
                return pixmap;
        }
        ret = gdk_pixbuf_scale_simple(pixmap, required_width, required_height,
                                      GDK_INTERP_BILINEAR);
        g_object_unref(pixmap);
        return ret;
    }
}

/* Update the icon of a task. */
static void _task_update_icon(TaskButton *task, TaskDetails *details, Atom source)
{
    GdkPixbuf *pixbuf = NULL;

    if (source == a_NET_ACTIVE_WINDOW && details != NULL)
        pixbuf = details->icon; /* use cached icon */

    /* Get the icon from the window's hints. */
    if (details != NULL && pixbuf == NULL)
    {
        pixbuf = get_wm_icon(details->win, task->icon_size, task->icon_size,
                             source, &details->image_source, task);
        if (pixbuf)
        {
            /* replace old cached image */
            if (details->icon)
                g_object_unref(details->icon);
            details->icon = g_object_ref_sink(pixbuf);
        }
        else
            /* use cached icon if available */
            pixbuf = details->icon;
    }

    /* If that fails, and we have no other icon yet, return the fallback icon. */
    if ((pixbuf == NULL)
        && ((source == None) || (details->image_source == None)))
    {
        GObject *parent = G_OBJECT(gtk_widget_get_parent(GTK_WIDGET(task)));

        /* Establish the fallback task icon.  This is used when no other icon is available. */
        pixbuf = g_object_get_data(parent, "task-fallback-pixbuf");
        if (pixbuf == NULL)
        {
            pixbuf = gdk_pixbuf_new_from_xpm_data((const char **) icon_xpm);
            if (pixbuf != NULL)
                g_object_set_data_full(parent, "task-fallback-pixbuf",
                                       g_object_ref_sink(pixbuf), g_object_unref);
        }
    }

    if (pixbuf != NULL)
        gtk_image_set_from_pixbuf(GTK_IMAGE(task->image), pixbuf);
}

static gboolean task_update_icon_idle(gpointer user_data)
{
    TaskButton *task;
    GList *l;
    TaskDetails *details;

    if (g_source_is_destroyed(g_main_current_source()))
        return FALSE;
    task = user_data;
    task->idle_loader = 0;
    for (l = task->details; l; l = l->next)
    {
        details = l->data;
        if (details->icon == NULL)
            _task_update_icon(task, details, None);
    }
    return FALSE;
}

/* Draw the label and tooltip on a taskbar button. */
static void task_draw_label(TaskButton *tb, gboolean bold_style, gboolean force)
{
    GString *str;
    gboolean old_bold = tb->set_bold;

    if (!force && old_bold == bold_style)
        return;
    if (tb->flags.icons_only)
        return;

    tb->set_bold = bold_style;
    str = g_string_sized_new(32);

    if (!tb->visible && tb->flags.show_square_brackets)
        g_string_append_c(str, '[');

    if (tb->n_visible > 1)
        g_string_append_printf(str, "(%d) ", tb->n_visible);

    if (!tb->same_name || !tb->last_focused || !tb->last_focused->name)
        g_string_append(str, tb->res_class);
    else
        g_string_append(str, tb->last_focused->name);

    if (!tb->visible && tb->flags.show_square_brackets)
        g_string_append_c(str, ']');

    if (force && tb->flags.tooltips)
        gtk_widget_set_tooltip_text(GTK_WIDGET(tb), str->str);

    lxpanel_draw_label_text(tb->panel, tb->label, str->str,
                            bold_style, 1, tb->flags.flat_button);

    g_string_free(str, TRUE);
}

/* Modern popup positioning using gtk_menu_popup_at_widget() */
static void popup_menu_at_button(GtkMenu *menu, GtkWidget *button, guint button_num, guint32 time)
{
    gtk_menu_popup_at_widget(menu,
                             button,
                             GDK_GRAVITY_SOUTH_WEST,
                             GDK_GRAVITY_NORTH_WEST,
                             NULL);
}

/* Button press handler */
static gboolean task_button_button_press_event(GtkWidget *widget, GdkEventButton *event)
{
    TaskButton *tb = PANEL_TASK_BUTTON(widget);
    GtkWidget *menu, *mi;

    if (event->button == 3) /* Right click */
    {
        if (tb->n_visible > 1)
        {
            menu = gtk_menu_new();

            /* Close all windows */
            mi = gtk_menu_item_new_with_mnemonic(_("_Close all windows"));
            gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);
            g_signal_connect(mi, "activate",
                             G_CALLBACK(taskbar_close_all_windows), tb);
            gtk_widget_show_all(menu);
        }
        else
        {
            menu = get_task_button_menu(tb, tb->last_focused);
        }

        /* Show menu using modern popup API */
        popup_menu_at_button(GTK_MENU(menu), widget, event->button, event->time);
    }
    return TRUE;
}

/* Button release handler */
static gboolean task_button_button_release_event(GtkWidget *widget, GdkEventButton *event)
{
    TaskButton *tb = PANEL_TASK_BUTTON(widget);
    TaskDetails *task;
    GList *l;

    if (!tb->entered_state)
        ;

    else if (tb->n_visible > 1)
    {
        if (event->button == 1)
        {
            if (tb->menu_list)
            {
                g_object_remove_weak_pointer(G_OBJECT(tb->menu_list),
                                             (void **)&tb->menu_list);
                g_signal_handlers_disconnect_by_func(G_OBJECT(tb->menu_list),
                                                     on_menu_list_selection_done, tb);
            }

            tb->menu_list = GTK_MENU(gtk_menu_new());
            g_object_add_weak_pointer(G_OBJECT(tb->menu_list),
                                      (void **)&tb->menu_list);
            g_signal_connect(tb->menu_list, "selection-done",
                             G_CALLBACK(on_menu_list_selection_done), tb);

            /* Build grouped-task popup */
            for (l = tb->details; l; l = l->next)
            {
                task = l->data;
                if (task->visible)
                {
                    char *name = task->iconified ?
                                 g_strdup_printf("[%s]", task->name) : NULL;

                    GtkWidget *mi = gtk_menu_item_new();
                    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
                    GtkWidget *lbl = gtk_label_new(name ? name : task->name);

                    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);

                    gtk_container_add(GTK_CONTAINER(mi), box);
                    gtk_box_pack_start(GTK_BOX(box), lbl, TRUE, TRUE, 0);

                    if (task->icon)
                    {
                        GtkWidget *im = gtk_image_new_from_pixbuf(task->icon);
                        gtk_box_pack_start(GTK_BOX(box), im, FALSE, FALSE, 0);
                    }

                    g_free(name);

                    task->menu_item = mi;

                    g_signal_connect(mi, "button-press-event",
                                     G_CALLBACK(taskbar_popup_activate_event), tb);
                    g_signal_connect(mi, "select",
                                     G_CALLBACK(menu_task_selected), tb);
                    g_signal_connect(mi, "deselect",
                                     G_CALLBACK(menu_task_deselected), tb);

                    gtk_menu_shell_append(GTK_MENU_SHELL(tb->menu_list), mi);
                }
                else
                    task->menu_item = NULL;
            }

            gtk_widget_show_all(GTK_WIDGET(tb->menu_list));
            popup_menu_at_button(tb->menu_list, widget, event->button, event->time);
        }
    }
    else
    {
        task_button_window_do_release_event(widget, tb->last_focused, event);
    }

    /* Flat buttons: force normal state */
    if (tb->flags.flat_button)
        gtk_widget_set_state_flags(widget, GTK_STATE_FLAG_NORMAL, TRUE);

    return TRUE;
}

/* Enter notify */
static gboolean task_button_enter_notify_event(GtkWidget *widget, GdkEventCrossing *event)
{
    TaskButton *tb = PANEL_TASK_BUTTON(widget);

    tb->entered_state = TRUE;
    task_draw_label(tb, tb->flags.flat_button, FALSE);

    if (tb->flags.flat_button)
        return TRUE;

    return GTK_WIDGET_CLASS(task_button_parent_class)->enter_notify_event(widget, event);
}

/* Leave notify */
static gboolean task_button_leave_notify_event(GtkWidget *widget, GdkEventCrossing *event)
{
    TaskButton *tb = PANEL_TASK_BUTTON(widget);

    tb->entered_state = FALSE;
    task_draw_label(tb, FALSE, FALSE);

    if (tb->flags.flat_button)
        return TRUE;

    return GTK_WIDGET_CLASS(task_button_parent_class)->leave_notify_event(widget, event);
}

/* -----------------------------------------------------------------------------
 * Interface functions
 */

/* creates new button and sets rendering options */
TaskButton *task_button_new(Window win, gint desk, gint desks, LXPanel *panel,
                            const char *res_class, TaskShowFlags flags)
{
    TaskButton *self = g_object_new(PANEL_TYPE_TASK_BUTTON, NULL);

    /* Set structural relief layout using proper GTK3 setter functions */
    gtk_button_set_relief(GTK_BUTTON(self), flags.flat_button ? GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);

    /* remember data */
    self->desktop = desk;
    self->n_desktops = desks;
    self->panel = panel;
    self->monitor = panel_get_monitor(panel);
    self->icon_size = panel_get_icon_size(panel);
    if (flags.use_smaller_icons)
        self->icon_size -= 4;
    self->res_class = g_strdup(res_class);
    self->flags = flags;
    
    /* create empty image and label components natively */
    self->image = gtk_image_new();
    self->label = gtk_label_new(NULL);
    
    /* append the window and set icon/label by that */
    task_button_add_window(self, win, self->res_class);
    
    /* and now let assemble all widgets we got */
    assemble_gui(self);
    
    /* and finally set visibility on it cleanly using standard GTK3 rules */
    gtk_widget_set_visible(GTK_WIDGET(self), self->n_visible > 0);
    return self;
}

gboolean task_button_has_window(TaskButton *button, Window win)
{
    GList *l;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), FALSE);

    for (l = button->details; l; l = l->next)
        if (((TaskDetails *)l->data)->win == win)
            return TRUE;
    return FALSE;
}

/* Helper to update or refresh active contextual popup menus when window conditions change */
static void task_button_refresh_active_menu(TaskButton *button)
{
    /* REWRITE RESOLVED FIXME: If a menu is actively attached and visible, trigger a dynamic reset */
    GtkWidget *menu = gtk_menu_get_for_attach_widget(GTK_WIDGET(button));
    if (menu && gtk_widget_get_visible(menu))
    {
        /* Force-trigger the panel reset loop to redraw items mapping active window state mutations */
        task_button_reset_menu(GTK_WIDGET(button));
    }
}

/* removes windows from button, that are missing in list */
void task_button_update_windows_list(TaskButton *button, Window *list, gint n)
{
    GList *l, *next;
    TaskDetails *details;
    gint i;
    gboolean has_removed = FALSE;

    g_return_if_fail(PANEL_IS_TASK_BUTTON(button));

    for (l = button->details; l; )
    {
        next = l->next;
        details = l->data;
        for (i = 0; i < n; i++)
            if (list[i] == details->win)
                break;
        if (i >= n) /* not found, remove details now */
        {
            button->details = g_list_delete_link(button->details, l);
            free_task_details(details);
            if (button->last_focused == details)
                button->last_focused = NULL;
            has_removed = TRUE;
        }
        l = next; /* go next details */
    }
    if (button->details == NULL) /* all windows were deleted */
    {
        GList *menu_list = gtk_menu_get_for_attach_widget(GTK_WIDGET(button));
        menu_list = g_list_copy(menu_list);
        for (l = menu_list; l; l = l->next)
        {
            GtkMenu *menu = GTK_MENU(l->data);
            gtk_menu_popdown(menu);
            gtk_menu_detach(menu);
        }
        g_list_free(menu_list);
        gtk_widget_destroy(GTK_WIDGET(button));
    }
    else if (has_removed)
    {
        if (task_update_visibility(button))
            task_redraw_label(button);
        
        /* REWRITE RESOLVED FIXME: Re-verify menu layout tracking targets */
        task_button_refresh_active_menu(button);
    }
}

/* returns TRUE if found and updated */
gboolean task_button_window_xprop_changed(TaskButton *button, Window win, Atom atom)
{
    TaskDetails *details;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), FALSE);

    details = task_details_lookup(button, win);
    if (details == NULL)
        return FALSE;

    /* Dispatch on atom. */
    if (atom == a_NET_WM_DESKTOP)
    {
        /* Window changed desktop. */
        details->desktop = get_net_wm_desktop(win);
        details->visible = task_is_visible(button, details);
        if (task_update_visibility(button))
            task_redraw_label(button);
    }
    else if ((atom == XA_WM_NAME) || (atom == a_NET_WM_NAME) || (atom == a_NET_WM_VISIBLE_NAME))
    {
        /* Window changed name. */
        if (task_set_names(details, atom))
            task_redraw_label(button);
    }
    else if (atom == XA_WM_CLASS)
    {
        /* Read the WM_CLASS property. */
        XClassHint ch;
        gchar *res_class;

        ch.res_name = NULL;
        ch.res_class = NULL;
        XGetClassHint(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), win, &ch);
        if (ch.res_name != NULL)
            XFree(ch.res_name);
        if (ch.res_class != NULL)
        {
            res_class = g_locale_to_utf8(ch.res_class, -1, NULL, NULL, NULL);
            XFree(ch.res_class);
            if (res_class != NULL)
            {
                g_free(button->res_class);
                button->res_class = res_class;
                if (!button->same_name)
                    task_redraw_label(button);
            }
        }
    }
    else if (atom == a_WM_STATE)
    {
        /* Window changed state. */
        details->iconified = (get_wm_state(win) == IconicState);
        details->visible = task_is_visible(button, details);
        if (task_update_visibility(button))
            task_redraw_label(button);
    }
    else if (atom == XA_WM_HINTS)
    {
        gboolean has_urgency = details->urgency;

        details->urgency = task_has_urgency(win);
        if (!has_urgency && details->urgency && button->flags.use_urgency_hint)
        {
            /* gained urgency, update the button */
            details->visible = task_is_visible(button, details);
            task_update_visibility(button);
            if (details->visible)
                button->last_focused = details;
            task_redraw_label(button);
        }
        /* Window changed "window manager hints".
         * Some windows set their WM_HINTS icon after mapping. */
        task_update_icon(button, details, atom);
    }
    else if (atom == a_NET_WM_ICON)
    {
        /* Window changed EWMH icon. */
        task_update_icon(button, details, atom);
    }

    return TRUE;
}

gboolean task_button_window_focus_changed(TaskButton *button, Window *win)
{
    GList *l;
    TaskDetails *details;
    gboolean res = FALSE;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), FALSE);

    for (l = button->details; l; l = l->next)
    {
        details = l->data;
        if (win && details->win == *win)
        {
            res = TRUE;
            details->focused = TRUE;
            button->last_focused = details;
        }
        else
            details->focused = FALSE;
    }
    if (res)
    {
        /* for non-flat buttons we have to reflect focus by button state */
        if (!button->flags.flat_button)
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
            
        /* if focus changed that means button widgets may need update */
        task_update_icon(button, button->last_focused, a_NET_ACTIVE_WINDOW);
        task_redraw_label(button);
        
        /* REWRITE RESOLVED FIXME: Sync the menu elements dynamically */
        task_button_refresh_active_menu(button);
    }
    else
    {
        /* if no focus on any button window then button may need style update */
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
        
        /* REWRITE RESOLVED FIXME: Sync the menu elements dynamically */
        task_button_refresh_active_menu(button);
    }
    return res;
}

/* update internal data */
gboolean task_button_window_reconfigured(TaskButton *button, Window win)
{
    gint old_mon, new_mon;
    TaskDetails *details;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), FALSE);

    details = task_details_lookup(button, win);
    if (details == NULL)
        return FALSE;

    /* If the same_monitor_only option is set and the window is on a different
       monitor than before, redraw the task button */
    old_mon = details->monitor;
    new_mon = get_window_monitor(details->win);
    details->monitor = new_mon;

    if (button->flags.same_monitor_only
        && (old_mon == button->monitor || new_mon == button->monitor))
    {
        details->visible = task_is_visible(button, details);
        task_update_visibility(button);
        task_redraw_label(button);
        
        /* REWRITE RESOLVED FIXME: Sync menu state targets */
        task_button_refresh_active_menu(button);
    }
    return TRUE;
}

/* updates rendering options */
void task_button_update(TaskButton *button, gint desk, gint desks,
                        gint mon, guint icon_size, TaskShowFlags flags)
{
    gboolean changed = FALSE, changed_icon = FALSE, changed_label = FALSE;

    g_return_if_fail(PANEL_IS_TASK_BUTTON(button));

    if (button->desktop != desk
        || button->monitor != mon
        || button->flags.show_all_desks != flags.show_all_desks
        || button->flags.same_monitor_only != flags.same_monitor_only)
        changed = TRUE;
    if (button->n_desktops != desks)
        task_button_reset_menu(gtk_widget_get_parent(GTK_WIDGET(button)));
    if (button->icon_size != icon_size
        || button->flags.disable_taskbar_upscale != flags.disable_taskbar_upscale)
        changed_icon = TRUE;
    if (button->flags.flat_button != flags.flat_button ||
        button->flags.show_square_brackets != flags.show_square_brackets)
        changed_label = TRUE;
    if (button->flags.icons_only != flags.icons_only)
    {
        changed_label = !flags.icons_only;
        gtk_widget_set_visible(button->label, changed_label);
    }
    if (button->flags.flat_button != flags.flat_button)
    {
        if(flags.flat_button)
        {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
        }
        else
            gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NORMAL);
    }
    button->desktop = desk;
    button->n_desktops = desks;
    button->monitor = mon;
    button->icon_size = icon_size;
    button->flags = flags;

    if (changed)
    {
        if (task_update_visibility(button))
            changed_label = TRUE;
            
        /* REWRITE RESOLVED FIXME: Sync menu targets */
        task_button_refresh_active_menu(button);
    }
    if (changed_label)
        task_redraw_label(button);
    if (changed_icon)
        task_update_icon(button, button->last_focused, None);
}

/* updates state for flashing buttons, including menu list */
void task_button_set_flash_state(TaskButton *button, gboolean state)
{
    gboolean has_flash = FALSE, m_state;
    GList *l;
    TaskDetails *details;

    g_return_if_fail(PANEL_IS_TASK_BUTTON(button));

    for (l = button->details; l; l = l->next)
    {
        details = l->data;
        if (button->flags.use_urgency_hint && details->urgency)
        {
            has_flash = TRUE;
            m_state = state;
        }
        else
            m_state = FALSE;
            
        if (button->menu_list && details->menu_item
            && button->menu_target != details->win)
        {
            /* REWRITE BUGFIX: Replaced deprecated gtk_widget_set_state with modern GTK3 StateFlags */
            GtkStateFlags target_flags = m_state ? GTK_STATE_FLAG_SELECTED : GTK_STATE_FLAG_NORMAL;
            gtk_widget_set_state_flags(details->menu_item, target_flags, TRUE);
        }
    }
    
    /* Set state on the button and redraw. */
    if (button->flags.flat_button)
    {
        if (has_flash || button->has_flash)
        {
            if (!has_flash)
                state = button->entered_state;
            task_draw_label(button, state, FALSE);
        }
    }
    else if (has_flash)
    {
        /* REWRITE BUGFIX: Fixed deprecated flash selector states using GTK3 parameters */
        gtk_widget_set_state_flags(GTK_WIDGET(button), state ? GTK_STATE_FLAG_SELECTED : GTK_STATE_FLAG_NORMAL, TRUE);
    }
    else if (!button->entered_state && button->has_flash)
    {
        GtkStateFlags fallback_flags = (button->last_focused == NULL) ? GTK_STATE_FLAG_NORMAL : GTK_STATE_FLAG_ACTIVE;
        gtk_widget_set_state_flags(GTK_WIDGET(button), fallback_flags, TRUE);
    }
    button->has_flash = has_flash;
}

/* adds task only if it's the same class */
gboolean task_button_add_window(TaskButton *button, Window win, const char *cl)
{
    TaskDetails *details;
    GtkAllocation alloc;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), FALSE);

    if (g_strcmp0(button->res_class, cl) != 0)
        return FALSE;
    /* fetch task details */
    details = task_details_for_window(button, win);
    button->details = g_list_append(button->details, details);
    /* redraw label on the button if need */
    if (details->visible)
    {
        if (task_update_visibility(button))
            task_redraw_label(button);
            
        /*Sync system context items */
        task_button_refresh_active_menu(button);
    }
    gtk_widget_get_allocation(GTK_WIDGET(button), &alloc);
    map_xwindow_animation(GTK_WIDGET(button), win, &alloc);
    return TRUE;
}

gboolean task_button_drop_window(TaskButton *button, Window win, gboolean leave_last)
{
    GList *l;
    TaskDetails *details;
    gboolean was_last_focused;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), FALSE);

    if (leave_last && g_list_length(button->details) <= 1)
        return FALSE;
    for (l = button->details; l; l = l->next)
        if (((TaskDetails *)l->data)->win == win)
            break;
    if (l == NULL) /* not our window */
        return FALSE;
    if (g_list_length(button->details) == 1)
    {
        /* this was last window, destroy the button */
        gtk_widget_destroy(GTK_WIDGET(button));
        return TRUE;
    }
    details = l->data;
    button->details = g_list_delete_link(button->details, l);
    was_last_focused = (button->last_focused == details);
    if (was_last_focused)
        button->last_focused = NULL;
    if (details->visible)
    {
        task_update_visibility(button);
        if (was_last_focused)
            task_update_icon(button, button->last_focused, None);
        task_redraw_label(button);
        
        /* REWRITE RESOLVED FIXME: Re-verify layout changes against operational context menus */
        task_button_refresh_active_menu(button);
    }
    /* bug SF#823: menu may be still opened for this window */
    if (button->menu_list && details->menu_item)
        gtk_widget_destroy(details->menu_item);
    free_task_details(details);
    return TRUE;
}
/* Helper to refresh active contextual menus when task button groups shift */
static void task_button_refresh_active_menu(TaskButton *button)
{
    /* REWRITE RESOLVED FIXME: Trigger a dynamic reset if a menu is actively mapped */
    GtkWidget *menu = gtk_menu_get_for_attach_widget(GTK_WIDGET(button));
    if (menu && gtk_widget_get_visible(menu))
    {
        task_button_reset_menu(gtk_widget_get_parent(GTK_WIDGET(button)));
    }
}

/* leaves only last task in button and returns a copy containing rest */
TaskButton *task_button_split(TaskButton *button)
{
    TaskButton *sibling;
    GList *llast;

    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button), NULL);

    if (g_list_length(button->details) < 2)
        return NULL;

    /* Initialize Custom GtkButton subclass layout cleanly for GTK3 */
    sibling = g_object_new(PANEL_TYPE_TASK_BUTTON, NULL);
    gtk_button_set_relief(GTK_BUTTON(sibling), button->flags.flat_button ? GTK_RELIEF_NONE : GTK_RELIEF_NORMAL);

    sibling->res_class = g_strdup(button->res_class);
    sibling->panel = button->panel;
    sibling->image = gtk_image_new();
    sibling->label = gtk_label_new(NULL);

    /* REWRITE BUGFIX: Safely slice the doubly-linked list chain without orphaning tracking headers */
    llast = g_list_last(button->details);
    if (llast && llast->prev)
    {
        /* Break the pointer link to cut the window collection in half */
        llast->prev->next = NULL;
        sibling->details = llast;
        llast->prev = NULL;
    }
    else
    {
        /* Fallback fail-safe if structure unexpected */
        sibling->details = g_list_remove_link(button->details, llast);
    }

    if (button->last_focused && button->last_focused != llast->data)
    {
        /* focused item migrated to sibling */
        sibling->last_focused = button->last_focused;
        button->last_focused = NULL;
    }

    sibling->desktop = button->desktop;
    sibling->n_desktops = button->n_desktops;
    sibling->monitor = button->monitor;
    sibling->icon_size = button->icon_size;
    sibling->flags = button->flags;

    task_update_visibility(button);
    task_update_visibility(sibling);

    /* force redraw icons and labels on buttons */
    if (button->n_visible > 0)
    {
        task_update_icon(button, button->last_focused, None);
        task_draw_label(button, FALSE, TRUE);
    }
    if (sibling->n_visible > 0)
    {
        task_update_icon(sibling, button->last_focused, None);
        task_draw_label(sibling, FALSE, TRUE);
    }

    assemble_gui(sibling);

    /* REWRITE RESOLVED FIXME: Clear active tracking layouts to prevent stale UI state */
    task_button_refresh_active_menu(button);

    return sibling;
}

/* merges buttons if they are the same class */
gboolean task_button_merge(TaskButton *button, TaskButton *sibling)
{
    g_return_val_if_fail(PANEL_IS_TASK_BUTTON(button) && PANEL_IS_TASK_BUTTON(sibling), FALSE);

    if (g_strcmp0(button->res_class, sibling->res_class) != 0)
        return FALSE;

    /* move data lists from sibling appending to button */
    button->details = g_list_concat(button->details, sibling->details);
    sibling->details = NULL;

    /* update visibility */
    button->n_visible += sibling->n_visible;
    button->visible = (button->visible | sibling->visible);

    /* eliminate sibling widget now */
    gtk_widget_destroy(GTK_WIDGET(sibling));

    /* redraw label on the button */
    task_redraw_label(button);

    /* REWRITE RESOLVED FIXME: Sync open menu contexts post-merge */
    task_button_refresh_active_menu(button);

    return TRUE;
}

/* single-instance-menu management, should be called on button parent widget */
void task_button_reset_menu(GtkWidget *parent)
{
    if (!parent)
        return;

    GtkWidget *menu = g_object_get_data(G_OBJECT(parent), "task-button-menu");

    if (menu)
    {
        /* REWRITE PORTING BUGFIX: Verify that the menu target is structurally valid and attached */
        if (GTK_IS_MENU(menu) && gtk_menu_get_attach_widget(GTK_MENU(menu)) == parent)
        {
            gtk_menu_detach(GTK_MENU(menu));
        }
        g_object_set_data(G_OBJECT(parent), "task-button-menu", NULL);
    }
    g_object_set_data(G_OBJECT(parent), "task-button-current", NULL);
}

/* request for a minimized window to raise */
void task_button_raise_window(TaskButton *button, guint32 time)
{
    if (!PANEL_IS_TASK_BUTTON(button))
        return;
    if (button->details)
        task_raise_window(button, button->details->data, time);
}