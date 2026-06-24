/*
 *      fm-action.h
 *
 *      Copyright 2014 Andriy Grytsenko (LStranger) <andrej@rep.kiev.ua>
 *
 *      This file is a part of the Libfm library.
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

#ifndef __FM_ACTION_H__
#define __FM_ACTION_H__ 1

#include <gio/gio.h>
#include <glib-object.h>

G_BEGIN_DECLS

/* Type Macros for GObject Registration System */
#define FM_TYPE_ACTION               (fm_action_get_type())
#define FM_ACTION(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), FM_TYPE_ACTION, FmAction))
#define FM_IS_ACTION(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_TYPE_ACTION))

#define FM_TYPE_ACTION_MENU          (fm_action_menu_get_type())
#define FM_ACTION_MENU(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), FM_TYPE_ACTION_MENU, FmActionMenu))
#define FM_IS_ACTION_MENU(obj)       (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_TYPE_ACTION_MENU))

#define FM_TYPE_ACTION_CACHE         (fm_action_cache_get_type())
#define FM_ACTION_CACHE(obj)         (G_TYPE_CHECK_INSTANCE_CAST((obj), FM_TYPE_ACTION_CACHE, FmActionCache))
#define FM_IS_ACTION_CACHE(obj)      (G_TYPE_CHECK_INSTANCE_TYPE((obj), FM_TYPE_ACTION_CACHE))

/* Forward Structural Declarations */
typedef struct _FmAction              FmAction;
typedef struct _FmActionMenu          FmActionMenu;
typedef struct _FmActionCache         FmActionCache;

typedef struct _FmActionClass         FmActionClass;
typedef struct _FmActionMenuClass     FmActionMenuClass;
typedef struct _FmActionCacheClass    FmActionCacheClass;

/* GObject Class Definition Blocks */
struct _FmActionClass {
    GObjectClass parent_class;
};

struct _FmActionMenuClass {
    GObjectClass parent_class;
};

struct _FmActionCacheClass {
    GObjectClass parent_class;
};

/* Type Fetching API Entry Points */
GType fm_action_get_type(void) G_GNUC_CONST;
GType fm_action_menu_get_type(void) G_GNUC_CONST;
GType fm_action_cache_get_type(void) G_GNUC_CONST;

/* Main Action Context Processing API */
FmActionMenu *fm_action_get_for_context(FmActionCache *cache, GFile *location, GList *files);
FmActionMenu *fm_action_get_for_location(FmActionCache *cache, GFile *location);
FmActionMenu *fm_action_get_for_toolbar(FmActionCache *cache, GFile *location);

/* Property Accessors */
const GList *fm_action_menu_get_children(FmActionMenu *menu);
const char *fm_action_get_suggested_shortcut(FmAction *action);
const char *fm_action_get_toolbar_label(FmAction *action);
const char *fm_action_get_startup_wm_class(FmAction *action);

/* Lifecycle Initialization */
FmActionCache *fm_action_cache_new(void);

G_END_DECLS

#endif /* __FM_ACTION_H__ */