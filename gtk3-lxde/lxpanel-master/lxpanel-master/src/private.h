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

#ifndef PRIVATE_H
#define PRIVATE_H

#include "plugin.h"
#include "conf.h"
#include "lxpanelctl.h"

#include <gmodule.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdio.h>
#include "panel.h"

#include "ev.h"

#if !GLIB_CHECK_VERSION(2, 40, 0)
# define g_info(...) g_log(G_LOG_DOMAIN, G_LOG_LEVEL_INFO, __VA_ARGS__)
#endif

/* -----------------------------------------------------------------------------
 * Definitions used by lxpanel main code internally */

/* Extracted from panel.h */
enum { ALIGN_NONE, ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT  };
enum { WIDTH_NONE, WIDTH_REQUEST, WIDTH_PIXEL, WIDTH_PERCENT };
enum { HEIGHT_NONE, HEIGHT_PIXEL, HEIGHT_REQUEST };

#define PANEL_ICON_SIZE               24    /* Default size of panel icons */
#define PANEL_HEIGHT_DEFAULT          26    /* Default height of horizontal panel */
#define PANEL_WIDTH_DEFAULT           150   /* Default "height" of vertical panel */
#define PANEL_HEIGHT_MAX              200   /* Maximum height of panel */
#define PANEL_HEIGHT_MIN              16    /* Minimum height of panel */
#define PANEL_ICON_HIGHLIGHT          0x202020  /* Constant to pass to icon loader */

typedef enum {
    PANEL_MOVE_STOP, /* initial state */
    PANEL_MOVE_DETECT, /* button pressed, detect drag */
    PANEL_MOVE_MOVING /* moving the plugin */
} PanelPluginMoveState;

typedef struct {
    int space_size;         /* size of space plugin if expandable */
    int plugin_center;      /* position of center of prev no-space plugin */
    GtkWidget * space;
    GtkWidget * plugin;
} PanelPluginMoveData;

/* to check if we are in LXDE */
extern gboolean is_in_lxde;

extern gchar *cprofile;

extern GSList* all_panels;

/* Context of a panel on a given edge. */
struct _Panel {
    char* name;
    LXPanel * topgwin;          /* Main panel window */
    Window topxwin;         /* Main panel's X window    */
    GdkDisplay * display;       /* Main panel's GdkDisplay */
    GtkStyleContext * style_ctx; /* GTK3 Replacement for GtkStyle * defstyle; */
    GtkIconTheme* icon_theme; /*Default icon theme*/

    GtkWidget * box;            /* Top level widget */

    GtkWidget *(*my_box_new) (gboolean, gint);
    GtkWidget *(*my_separator_new) (void);

    void *bg; /* unused since 0.8.0 */
    int alpha;
    guint32 tintcolor;
    guint32 fontcolor;
    GdkRGBA gtintcolor;      /* GTK3 Replacement for GdkColor */
    GdkRGBA gfontcolor;      /* GTK3 Replacement for GdkColor */

    int ax, ay, aw, ah;  /* preferred allocation of a panel */
    int cx, cy, cw, ch;  /* current allocation (as reported by configure event) allocation */
    int align, edge, margin;
    guint orientation;
    int widthtype, width;
    int heighttype, height;
    gint monitor;
    gulong strut_size;          /* Values for WM_STRUT_PARTIAL */
    gulong strut_lower;
    gulong strut_upper;
    int strut_edge;

    guint config_changed : 1;
    guint self_destroy : 1;
    guint setdocktype : 1;
    guint setstrut : 1;
    guint round_corners : 1;
    guint usefontcolor : 1;
    guint usefontsize : 1;
    guint fontsize;
    guint transparent : 1;
    guint background : 1;
    guint spacing;

    guint autohide : 1;
    guint visible : 1;
    int height_when_hidden;
    guint hide_timeout;
    int icon_size;          /* Icon size */

    int desknum;
    int curdesk;
    gulong *workarea; /* unused since 0.8.0 */
    int wa_len; /* unused since 0.8.0 */

    char* background_file;

    PanelConf * config;                 /* Panel configuration data */
    GSList * system_menus;      /* List of plugins having menus: deprecated */

    GtkWidget* plugin_pref_dialog;  /* Plugin preference dialog */
    GtkWidget* pref_dialog;     /* preference dialog */
    GtkWidget* margin_control;      /* Margin control in preference dialog */
    GtkWidget* height_label;        /* Label of height control */
    GtkWidget* width_label;     /* Label of width control */
    GtkWidget* alignment_left_label;    /* Label of alignment: left control */
    GtkWidget* alignment_right_label;   /* Label of alignment: right control */
    GtkWidget* height_control;      /* Height control in preference dialog */
    GtkWidget* width_control;       /* Width control in preference dialog */
    GtkWidget* strut_control;       /* Reserve space in preference dialog */
    GtkWidget* edge_bottom_button;
    GtkWidget* edge_top_button;
    GtkWidget* edge_left_button;
    GtkWidget* edge_right_button;

    guint initialized : 1;              /* Should be grouped better later, */
    guint ah_far : 1;                   /* placed here for binary compatibility */
    guint ah_state : 3;
    guint background_update_queued;
    guint strut_update_queued;
    guint mouse_timeout;
    guint reconfigure_queued;

    cairo_surface_t *surface;           /* Panel background */

    PanelPluginMoveState move_state;    /* Plugin movement (drag&drop) support */
    int move_x, move_y;
    int move_diff;
    GdkDevice * move_device;
    GtkWidget * move_plugin;            /* widgets involved in movement */
    PanelPluginMoveData move_before;
    PanelPluginMoveData move_after;
};

typedef struct {
    char *name;
    char *disp_name;
    void (*cmd)(void);
} Command;

/* In GTK3, use gdk_window_get_user_data() or keep application-specific lookup if still using X11 */
#define FBPANEL_WIN(win)  gdk_window_lookup(win)

/* Extracted from misc.h */
typedef struct {
    int num;
    gchar *str;
} pair;

extern pair align_pair[];
extern pair edge_pair[];
extern pair width_pair[];
extern pair height_pair[];
extern pair bool_pair[];

int str2num(pair *p, const gchar *str, int defval);
const gchar *num2str(pair *p, int num, const gchar *defval);

#ifdef __LXPANEL_INTERNALS__
static inline char *_system_config_file_name(const char *dir, const char *file_name)
{
    return g_build_filename(dir, "lxpanel", cprofile, file_name, NULL);
}

static inline char *_old_system_config_file_name(const char *file_name)
{
    return g_build_filename(PACKAGE_DATA_DIR "/profile", cprofile, file_name, NULL);
}

static inline char *_user_config_file_name(const char *name1, const char *name2)
{
    return g_build_filename(g_get_user_config_dir(), "lxpanel", cprofile, name1,
                            name2, NULL);
}
#endif

void load_global_config(void);
void free_global_config(void);

/* FIXME: optional definitions */
#define STATIC_SEPARATOR
#define STATIC_LAUNCHBAR
#define STATIC_LAUNCHTASKBAR
#define STATIC_DCLOCK
#define STATIC_WINCMD
#define STATIC_DIRMENU
#define STATIC_TASKBAR
#define STATIC_PAGER
#define STATIC_TRAY
#define STATIC_MENU
#define STATIC_ICONS

/* Plugins management - new style */
void lxpanel_prepare_modules(void);
void lxpanel_unload_modules(void);

GHashTable *lxpanel_get_all_types(void); /* transfer none */
void _lxpanel_remove_plugin(LXPanel *p, GtkWidget *plugin); /* no destroy dialog */

extern GQuark lxpanel_plugin_qinit; /* access to LXPanelPluginInit data */
#define PLUGIN_CLASS(_i) ((LXPanelPluginInit*)g_object_get_qdata(G_OBJECT(_i),lxpanel_plugin_qinit))

extern GQuark lxpanel_plugin_qconf; /* access to config_setting_t data */

#define PLUGIN_PANEL(_i) ((LXPanel*)gtk_widget_get_toplevel(_i))

gboolean _class_is_present(const LXPanelPluginInit *init);

LXPanel* panel_new(const char* config_file, const char* config_name);

void _panel_show_config_dialog(LXPanel *panel, GtkWidget *p, GtkWidget *dlg);

void _calculate_position(LXPanel *panel, GdkRectangle *rect);

void _panel_establish_autohide(LXPanel *p);
void _panel_set_wm_strut(LXPanel *p);
void _panel_set_panel_configuration_changed(LXPanel *p);
void _panel_queue_update_background(LXPanel *p);
void _panel_emit_icon_size_changed(LXPanel *p);
void _panel_emit_font_changed(LXPanel *p);

void panel_configure(LXPanel* p, int sel_page);
gboolean panel_edge_available(Panel* p, int edge, gint monitor);
gboolean _panel_edge_can_strut(LXPanel *panel, int edge, gint monitor, gulong *size);
void restart(void);
void logout(void);
void gtk_run(void);

/* GTK3 update: event structs should generally use generic GdkEvent* or specific GTK3 versions */
gboolean _lxpanel_button_release(GtkWidget *widget, GdkEventButton *event);
gboolean _lxpanel_motion_notify(GtkWidget *widget, GdkEventMotion *event);


/* -----------------------------------------------------------------------------
 * Deprecated declarations. Kept for compatibility with old code plugins. */

extern Command commands[];

/* Extracted from panel.h */
extern int verbose;

extern void panel_destroy(Panel *p);
extern void panel_adjust_geometry_terminology(Panel *p);
extern void panel_determine_background_pixmap(Panel * p, GtkWidget * widget, GdkWindow * window);
extern void panel_draw_label_text(Panel * p, GtkWidget * label, const char * text,
                                  gboolean bold, float custom_size_factor,
                                  gboolean custom_color);
extern void panel_establish_autohide(Panel *p);
extern void panel_image_set_from_file(Panel * p, GtkWidget * image, const char * file);
extern gboolean panel_image_set_icon_theme(Panel * p, GtkWidget * image, const gchar * icon);
extern void panel_set_wm_strut(Panel *p);
extern void panel_set_dock_type(Panel *p);
extern void panel_set_panel_configuration_changed(Panel *p);
extern void panel_update_background( Panel* p );

/* if current window manager is EWMH conforming. */
extern gboolean is_ewmh_supported;

void get_button_spacing(GtkRequisition *req, GtkContainer *parent, gchar *name);

/* Recreates a box layout in GTK3 */
GtkWidget* recreate_box( GtkBox* oldbox, GtkOrientation orientation );

extern const char* lxpanel_get_file_manager();


/* Extracted from misc.h */
typedef struct _Plugin Plugin;

enum { LINE_NONE, LINE_BLOCK_START, LINE_BLOCK_END, LINE_VAR };

typedef struct {
    int num, len, type;
    gchar str[256];
    gchar *t[3];
} line;

void calculate_position(Panel *np);

extern int lxpanel_get_line(char **fp, line *s);
extern int lxpanel_put_line(FILE* fp, const char* format, ...);
#define lxpanel_put_str(fp, name, val) (G_UNLIKELY( !(val) || !*(val) )) ? 0 : lxpanel_put_line(fp, "%s=%s", name, val)
#define lxpanel_put_bool(fp, name, val) lxpanel_put_line(fp, "%s=%c", name, (val) ? '1' : '0')
#define lxpanel_put_int(fp, name, val) lxpanel_put_line(fp, "%s=%d", name, val)

GtkWidget *_gtk_image_new_from_file_scaled(const gchar *file, gint width,
                                           gint height, gboolean keep_ratio);

GtkWidget * fb_button_new_from_file(
    const gchar * image_file, int width, int height, gulong highlight_color, gboolean keep_ratio);
GtkWidget * fb_button_new_from_file_with_label(
    const gchar * image_file, int width, int height, gulong highlight_color, gboolean keep_ratio, Panel * panel, const gchar * label);

void fb_button_set_from_file(GtkWidget* btn, const char* img_file, gint width, gint height, gboolean keep_ratio);

char* translate_exec_to_cmd( const char* exec, const char* icon,
                             const char* title, const char* fpath );

void show_error( GtkWindow* parent_win, const char* msg );

gboolean spawn_command_async(GtkWindow *parent_window, gchar const* workdir,
        gchar const* cmd);

/* Parameters: const char* name, gpointer ret_value, GType type, ....NULL */
GtkWidget* create_generic_config_dlg( const char* title, GtkWidget* parent,
                              GSourceFunc apply_func, Plugin * plugin,
                      const char* name, ... );

extern GtkMenu* lxpanel_get_panel_menu( Panel* panel, Plugin* plugin, gboolean use_sub_menu );

gboolean lxpanel_launch_app(const char* exec, GList* files, gboolean in_terminal, char const* in_workdir);

extern GdkPixbuf* lxpanel_load_icon( const char* name, int width, int height, gboolean use_fallback );

void Xclimsg(Window win, Atom type, long l0, long l1, long l2, long l3, long l4);


/* Extracted from plugin.h */
struct _Plugin;

#define PLUGINCLASS_VERSION 1
#define PLUGINCLASS_VERSIONING \
    .structure_size = sizeof(PluginClass), \
    .structure_version = PLUGINCLASS_VERSION

typedef struct {
    unsigned short structure_size;
    unsigned short structure_version;

    char * fname;
    int count;
    GModule * gmodule;

    int dynamic : 1;
    int unused_invisible : 1;
    int not_unloadable : 1;
    int one_per_system : 1;
    int one_per_system_instantiated : 1;
    int expand_available : 1;
    int expand_default : 1;

    char * type;
    char * name;
    char * version;
    char * description;

    int (*constructor)(struct _Plugin * plugin, char ** fp);
    void (*destructor)(struct _Plugin * plugin);
    void (*config)(struct _Plugin * plugin, GtkWindow * parent);
    void (*save)(struct _Plugin * plugin, FILE * fp);
    void (*panel_configuration_changed)(struct _Plugin * plugin);
} PluginClass;

struct _Plugin {
    PluginClass * class;
    Panel * panel;
    GtkWidget * pwid;
    int expand;
    int padding;
    int border;
    gpointer priv;
};

/* Plugins management - deprecated style, for backward compatibility */
extern gboolean plugin_button_press_event(GtkWidget *widget, GdkEventButton *event, Plugin *plugin);
extern void plugin_adjust_popup_position(GtkWidget * popup, Plugin * plugin);
extern void plugin_popup_set_position_helper(Plugin * p, GtkWidget * near, GtkWidget * popup, GtkRequisition * popup_req, gint * px, gint * py);

extern void lxpanel_image_set_from_file(LXPanel * p, GtkWidget * image, const char * file);
extern gboolean lxpanel_image_set_icon_theme(LXPanel * p, GtkWidget * image, const gchar * icon);

#endif