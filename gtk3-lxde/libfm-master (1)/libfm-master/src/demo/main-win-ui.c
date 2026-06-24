/*
 *      main-win-ui.c
 *      
 *      Copyright 2009 PCMan <pcman.tw@gmail.com>
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

/* this file is included by main-win.c */

#include <gtk/gtk.h>
#include "main-win.h"
#include "main-win-ui.h"

/* Menubar, toolbar, and accelerators definition.
 * Icons are now driven by icon names / themes, not GTK_STOCK_*.
 */
static const char main_menu_xml[] =
"<menubar>"
  "<menu action='FileMenu'>"
    "<menuitem action='New'/>"
    "<menuitem action='Close'/>"
  "</menu>"
  "<menu action='EditMenu'>"
    "<menuitem action='Cut'/>"
    "<menuitem action='Copy'/>"
    "<menuitem action='Paste'/>"
    "<menuitem action='Del'/>"
    "<separator/>"
    "<menuitem action='Rename'/>"
    "<menuitem action='Link'/>"
    "<menuitem action='MoveTo'/>"
    "<menuitem action='CopyTo'/>"
    "<separator/>"
    "<menuitem action='SelAll'/>"
    "<menuitem action='InvSel'/>"
    "<separator/>"
    "<menuitem action='Pref'/>"
  "</menu>"
  "<menu action='GoMenu'>"
    "<menuitem action='Prev'/>"
    "<menuitem action='Next'/>"
    "<menuitem action='Up'/>"
    "<separator/>"
    "<menuitem action='Home'/>"
    "<menuitem action='Desktop'/>"
    "<menuitem action='Trash'/>"
    "<menuitem action='Apps'/>"
    "<menuitem action='Computer'/>"
    "<menuitem action='Network'/>"
    "<separator/>"
    "<menuitem action='Search'/>"
  "</menu>"
  "<menu action='BookmarksMenu'>"
    "<menuitem action='AddBookmark'/>"
  "</menu>"
  "<menu action='ViewMenu'>"
    "<menuitem action='Reload'/>"
    "<menuitem action='ShowHidden'/>"
    "<separator/>"
    "<placeholder name='ph1'/>"
    "<separator/>"
    "<menu action='PathMode'>"
      "<menuitem action='PathEntry'/>"
      "<menuitem action='PathBar'/>"
    "</menu>"
    "<separator/>"
    "<menu action='Sort'>"
      "<menuitem action='Desc'/>"
      "<menuitem action='Asc'/>"
      "<separator/>"
      "<menuitem action='ByName'/>"
      "<menuitem action='ByMTime'/>"
    "</menu>"
  "</menu>"
  "<menu action='HelpMenu'>"
    "<menuitem action='About'/>"
  "</menu>"
"</menubar>"
"<toolbar>"
    "<toolitem action='New'/>"
    "<toolitem action='Prev'/>"
    "<toolitem action='Next'/>"
    "<toolitem action='Up'/>"
    "<toolitem action='Home'/>"
    "<toolitem action='Go'/>"
"</toolbar>"
"<accelerator action='Location'/>"
"<accelerator action='Location2'/>";

/* NOTE:
 * stock_id field is now NULL everywhere.
 * Icons should be provided via icon-name on proxies (menu items / tool items)
 * or via CSS / theme, not via GTK_STOCK_*.
 */
static GtkActionEntry main_win_actions[] =
{
    /* Menus */
    { "FileMenu",      NULL,              N_("_File"),      NULL,       NULL, NULL },
        { "New",       NULL,              N_("_New Window"), "<Ctrl>N", NULL, G_CALLBACK(on_new_win) },
        { "Close",     NULL,              N_("_Close Window"), "<Ctrl>W", NULL, G_CALLBACK(on_close_win) },

    { "EditMenu",      NULL,              N_("_Edit"),      NULL,       NULL, NULL },
        { "Cut",       NULL,              N_("Cut"),        NULL,       NULL, G_CALLBACK(bounce_action) },
        { "Copy",      NULL,              N_("Copy"),       NULL,       NULL, G_CALLBACK(bounce_action) },
        { "Paste",     NULL,              N_("Paste"),      NULL,       NULL, G_CALLBACK(bounce_action) },
        { "Del",       NULL,              N_("Delete"),     NULL,       NULL, G_CALLBACK(bounce_action) },
        { "Rename",    NULL,              N_("Rename"),     "F2",       NULL, G_CALLBACK(on_rename) },
        { "Link",      NULL,              N_("Create Symlink"), NULL,   NULL, NULL },
        { "MoveTo",    NULL,              N_("Move To..."), NULL,       NULL, G_CALLBACK(on_move_to) },
        { "CopyTo",    NULL,              N_("Copy To..."), NULL,       NULL, G_CALLBACK(on_copy_to) },
        { "SelAll",    NULL,              N_("Select All"), NULL,       NULL, G_CALLBACK(bounce_action) },
        { "InvSel",    NULL,              N_("Invert Selection"), NULL, NULL, G_CALLBACK(bounce_action) },
        { "Pref",      NULL,              N_("Preferences"), NULL,      NULL, NULL },

    { "ViewMenu",      NULL,              N_("_View"),      NULL,       NULL, NULL },
        { "Reload",    NULL,              N_("Reload"),     "F5",       NULL, G_CALLBACK(on_reload) },
        { "PathMode",  NULL,              N_("_Path Bar"),  NULL,       NULL, NULL },
        { "Sort",      NULL,              N_("_Sort Files"), NULL,      NULL, NULL },

    { "HelpMenu",      NULL,              N_("_Help"),      NULL,       NULL, NULL },
        { "About",     NULL,              N_("About"),      NULL,       NULL, G_CALLBACK(on_about) },

    { "GoMenu",        NULL,              N_("_Go"),        NULL,       NULL, NULL },
        { "Prev",      NULL,              N_("Previous Folder"), "<Alt>Left",  N_("Previous Folder"),  G_CALLBACK(on_go_back) },
        { "Next",      NULL,              N_("Next Folder"),     "<Alt>Right", N_("Next Folder"),      G_CALLBACK(on_go_forward) },
        { "Up",        NULL,              N_("Parent Folder"),   "<Alt>Up",    N_("Go to parent Folder"), G_CALLBACK(on_go_up) },
        { "Home",      NULL,              N_("Home Folder"),     "<Alt>Home", N_("Home Folder"),       G_CALLBACK(on_go_home) },
        { "Desktop",   NULL,              N_("Desktop"),         NULL,        N_("Desktop Folder"),    G_CALLBACK(on_go_desktop) },
        { "Computer",  NULL,              N_("Devices"),         NULL,        NULL,                    G_CALLBACK(on_go_computer) },
        { "Trash",     NULL,              N_("Trash Can"),       NULL,        NULL,                    G_CALLBACK(on_go_trash) },
        { "Network",   NULL,              N_("Network"),         NULL,        NULL,                    G_CALLBACK(on_go_network) },
        { "Apps",      NULL,              N_("Applications"),    NULL,        N_("Installed Applications"), G_CALLBACK(on_go_apps) },
        { "Go",        NULL,              N_("Go"),              NULL,        NULL,                    G_CALLBACK(on_go) },
        { "Search",    NULL,              N_("Find Files"),      NULL,        NULL,                    G_CALLBACK(on_search) },

    { "BookmarksMenu", NULL,              N_("_Bookmarks"), NULL,       NULL, NULL },
        { "AddBookmark", NULL,            N_("Add To Bookmarks"), NULL, N_("Add To Bookmarks"), NULL },

    /* accelerators only, no visible UI */
    { "Location",  NULL, NULL, "<Alt>d",  NULL, G_CALLBACK(on_location) },
    { "Location2", NULL, NULL, "<Ctrl>L", NULL, G_CALLBACK(on_location) },
};

static GtkToggleActionEntry main_win_toggle_actions[] =
{
    { "ShowHidden", NULL, N_("Show _Hidden"), "<Ctrl>H", NULL, G_CALLBACK(on_show_hidden), FALSE }
};

static GtkRadioActionEntry main_win_sort_type_actions[] =
{
    { "Asc",  NULL, N_("Ascending"),   NULL, NULL, GTK_SORT_ASCENDING },
    { "Desc", NULL, N_("Descending"),  NULL, NULL, GTK_SORT_DESCENDING },
};

static GtkRadioActionEntry main_win_sort_by_actions[] =
{
    { "ByName",  NULL, N_("By _Name"),              NULL, NULL, FM_FOLDER_MODEL_COL_NAME },
    { "ByMTime", NULL, N_("By _Modification Time"), NULL, NULL, FM_FOLDER_MODEL_COL_MTIME }
};

static GtkRadioActionEntry main_win_pathbar_actions[] =
{
    { "PathEntry", NULL, N_("_Location"), NULL, NULL, 0 },
    { "PathBar",   NULL, N_("_Buttons"),  NULL, NULL, 1 }
};

static const char folder_menu_xml[] =
"<popup>"
  "<placeholder name='ph1'>"
    "<menuitem action='NewWin'/>"
  "</placeholder>"
"</popup>";

/* Popup menu actions */
static GtkActionEntry folder_menu_actions[] =
{
    { "NewWin",  NULL, N_("Open in New Window"), NULL, NULL, G_CALLBACK(on_open_in_new_win) },
    { "Search",  NULL, N_("Find Files"),         NULL, NULL, NULL } /* reserved / unused callback */
};

/* Accessors so main-win.c can use these tables */

GtkActionEntry *
fm_main_win_get_actions (guint *n_actions)
{
    if (n_actions)
        *n_actions = G_N_ELEMENTS (main_win_actions);
    return main_win_actions;
}

GtkToggleActionEntry *
fm_main_win_get_toggle_actions (guint *n_toggle)
{
    if (n_toggle)
        *n_toggle = G_N_ELEMENTS (main_win_toggle_actions);
    return main_win_toggle_actions;
}

GtkRadioActionEntry *
fm_main_win_get_sort_type_actions (guint *n_radio)
{
    if (n_radio)
        *n_radio = G_N_ELEMENTS (main_win_sort_type_actions);
    return main_win_sort_type_actions;
}

GtkRadioActionEntry *
fm_main_win_get_sort_by_actions (guint *n_radio)
{
    if (n_radio)
        *n_radio = G_N_ELEMENTS (main_win_sort_by_actions);
    return main_win_sort_by_actions;
}

GtkRadioActionEntry *
fm_main_win_get_pathbar_actions (guint *n_radio)
{
    if (n_radio)
        *n_radio = G_N_ELEMENTS (main_win_pathbar_actions);
    return main_win_pathbar_actions;
}

GtkActionEntry *
fm_folder_menu_get_actions (guint *n_actions)
{
    if (n_actions)
        *n_actions = G_N_ELEMENTS (folder_menu_actions);
    return folder_menu_actions;
}

const char *
fm_main_win_get_main_menu_xml (void)
{
    return main_menu_xml;
}

const char *
fm_main_win_get_folder_menu_xml (void)
{
    return folder_menu_xml;
}
