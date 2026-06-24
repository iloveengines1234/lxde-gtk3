/**
 * Copyright (c) 2010 LxDE Developers, see the file AUTHORS for details.
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

#ifndef __LXSESSION_LOGOUT_DBUS_INTERFACE_H__
#define __LXSESSION_LOGOUT_DBUS_INTERFACE_H__

#include <glib.h>

G_BEGIN_DECLS

/* * DBus backend subsystem wrappers for session state suspension and shutdown
 * Compatible with GTK 3.24.52 and standard GIO frameworks.
 */

/* Interface to ConsoleKit for suspend, hibernate, shutdown, and reboot. */
gboolean dbus_ConsoleKit_CanPowerOff(void);
gboolean dbus_ConsoleKit_CanReboot(void);
gboolean dbus_ConsoleKit_CanSuspend(void);
gboolean dbus_ConsoleKit_CanHibernate(void);
void dbus_ConsoleKit_PowerOff(GError **error);
void dbus_ConsoleKit_Reboot(GError **error);
void dbus_ConsoleKit_Suspend(GError **error);
void dbus_ConsoleKit_Hibernate(GError **error);

/* Interface to UPower for suspend and hibernate. */
gboolean dbus_UPower_CanSuspend(void);
gboolean dbus_UPower_CanHibernate(void);
gboolean dbus_UPower_Suspend(GError **error);
gboolean dbus_UPower_Hibernate(GError **error);

/* Interface to systemd for suspend, hibernate, shutdown, and reboot. */
gboolean dbus_systemd_CanPowerOff(void);
gboolean dbus_systemd_CanReboot(void);
gboolean dbus_systemd_CanSuspend(void);
gboolean dbus_systemd_CanHibernate(void);
void dbus_systemd_PowerOff(GError **error);
void dbus_systemd_Reboot(GError **error);
void dbus_systemd_Suspend(GError **error);
void dbus_systemd_Hibernate(GError **error);

/* Interface to LightDM for user session switching. */
gboolean dbus_Lightdm_SwitchToGreeter(GError **error);

/* Interface to LXDE for global desktop logout execution. */
gboolean dbus_LXDE_Logout(GError **error);

G_END_DECLS

#endif /* __LXSESSION_LOGOUT_DBUS_INTERFACE_H__ */