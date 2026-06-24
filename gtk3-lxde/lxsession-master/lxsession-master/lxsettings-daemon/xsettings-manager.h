/*
 * Copyright © 2001 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Updated by iloveengines1234 to port to gtk3
 * Author:  Owen Taylor, Red Hat, Inc.
 */

#ifndef __XSETTINGS_MANAGER_H__
#define __XSETTINGS_MANAGER_H__

#pragma once

#include <glib.h>
#include "xsettings-common.h"

G_BEGIN_DECLS

/* Modern Fix: Forward declare basic X11 types to completely prevent leaking 
 * internal Xlib structural pollution into downstream GTK 3 environment files. 
 */
typedef struct _XDisplay Display;
typedef unsigned long XID;
typedef XID Window;
typedef unsigned long Atom;
typedef union _XEvent XEvent;

/* Safe C-Standard mapping for X11 primitive Boolean evaluations */
#ifndef Bool
typedef int Bool;
#endif

typedef struct _XSettingsManager XSettingsManager;

typedef void (*XSettingsTerminateFunc) (void *cb_data);

/**
 * xsettings_manager_check_running:
 * @display: Open X Display connection pipeline.
 * @screen: Target layout screen number index.
 *
 * Scans the target display workspace selection locks to see if an external 
 * XSettings daemon instance has already laid claim to the screen.
 *
 * Returns: True if an active instance is already owning the selection.
 */
Bool xsettings_manager_check_running (Display *display,
                                      int      screen);

/**
 * xsettings_manager_new:
 * @display: Open X Display connection pipeline.
 * @screen: Target layout screen number index.
 * @terminate: Callback hook executed if selection ownership is forcefully revoked.
 * @cb_data: Context user data passed into the termination callback.
 *
 * Generates an internal manager monitoring context, sets up selection 
 * ownership claims, and broadcasts validation signals to downstream apps.
 *
 * Returns: Freshly instantiated context pointer, or NULL on memory exhaustion.
 */
XSettingsManager *xsettings_manager_new (Display                *display,
                                         int                     screen,
                                         XSettingsTerminateFunc  terminate,
                                         void                   *cb_data);

void   xsettings_manager_destroy       (XSettingsManager *manager);
Window xsettings_manager_get_window    (XSettingsManager *manager);
Bool   xsettings_manager_process_event (XSettingsManager *manager,
                                        XEvent           *xev);

XSettingsResult xsettings_manager_delete_setting (XSettingsManager *manager,
                                                  const char       *name);
XSettingsResult xsettings_manager_set_setting    (XSettingsManager *manager,
                                                  XSettingsSetting *setting);
XSettingsResult xsettings_manager_set_int        (XSettingsManager *manager,
                                                  const char       *name,
                                                  int               value);
XSettingsResult xsettings_manager_set_string     (XSettingsManager *manager,
                                                  const char       *name,
                                                  const char       *value);
XSettingsResult xsettings_manager_set_color      (XSettingsManager *manager,
                                                  const char       *name,
                                                  XSettingsColor   *value);

/**
 * xsettings_manager_notify:
 * @manager: Target settings management workspace reference.
 *
 * Flushes all pending visual settings updates over to the property canvas,
 * triggering automatic theme changes in running client applications.
 *
 * Returns: XSETTINGS_SUCCESS on operational validation, or error token.
 */
XSettingsResult xsettings_manager_notify         (XSettingsManager *manager);

G_END_DECLS

#endif /* __XSETTINGS_MANAGER_H__ */