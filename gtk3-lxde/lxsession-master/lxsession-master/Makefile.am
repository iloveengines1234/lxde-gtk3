# Makefile.am
# Process this file with automake to produce Makefile.in
# Updated by iloveengines1234 to port completely to GTK 3.24.52 / Modern Autotools

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}
AUTOMAKE_OPTIONS = subdir-objects

NULL =

SUBDIRS = data man po

# Base execution paths
bin_PROGRAMS = \
	lxsettings-daemon/lxsettings-daemon \
	xdg-autostart/lxsession-xdg-autostart \
	lxsession/lxsession \
	$(NULL)

# Core runtime utility scripts
scripts = \
	lxlock/lxlock \
	lxsession-default/lxsession-default \
	lxsession-default/lxsession-default-terminal \
	$(NULL)

bin_SCRIPTS = $(scripts)

pkglibexec_SCRIPTS = lxsession-utils/lxsession-xsettings

noinst_DATA = \
	vapi/config.vapi \
	vapi/lxclipboard.vapi \
	vapi/lxpolkit.vapi \
	vapi/lxsession-edit.vapi \
	vapi/lxsettings-daemon.vapi \
	vapi/xdg-autostart.vapi \
	$(NULL)

# Modern Fix: Handled transient Vala C outputs carefully to ensure parallel 
# generation pipelines do not clear active build objects mid-run.
CLEANFILES = \
	lxclipboard/main.c \
	lxpolkit/main.c \
	lxsession-db/main.c \
	lxsession-db/desktop-files-backend.c \
	lxsession-default-apps/autostart.c \
	lxsession-default-apps/main.c \
	lxsession-default-apps/combobox.c \
	lxsession-default-apps/dbus-backend.c \
	lxsession-default-apps/utils.c \
	lxsession/main.c \
	lxsession/app.c \
	lxsession/autostart.c \
	lxsession/conffiles.c \
	lxsession/control.c \
	lxsession/dbus-common.c \
	lxsession/dbus-gnome-session.c \
	lxsession/dbus-lxde-session.c \
	lxsession/environement.c \
	lxsession/notifications.c \
	lxsession/options.c \
	lxsession/process.c \
	lxsession/settings.c \
	lxsession/utils.c \
	lxsettings-daemon/main.c \
	xdg-autostart/main.c \
	*.vala.stamp \
	$(NULL)

EXTRA_DIST = \
	$(noinst_DATA) \
	$(scripts) \
	lxsession-utils/lxsession-xsettings \
	autogen.sh \
	lxclipboard/clipboard.h \
	lxpolkit/lxpolkit.h \
	lxpolkit/lxpolkit-listener.h \
	lxsettings-daemon/settings-daemon.h \
	lxsettings-daemon/xevent.h \
	lxsettings-daemon/xsettings-common.h \
	lxsettings-daemon/xsettings-manager.h \
	lxsettings-daemon/xutils.h \
	lxsession-edit/lxsession-edit-common.h \
	lxsession-logout/lxsession-logout-dbus-interface.h \
	xdg-autostart/xdg-autostart.h \
	$(NULL)

bin_PROGRAMS += \
	lxpolkit/lxpolkit \
	lxsession-db/lxsession-db \
	lxsession-edit/lxsession-edit \
	lxsession-logout/lxsession-logout \
	lxsession-default-apps/lxsession-default-apps \
	lxclipboard/lxclipboard

# --- Component Rules ---

# Clipboard Utility
lxclipboard_lxclipboard_SOURCES = \
	lxclipboard/main.vala \
	lxclipboard/clipboard.c \
	$(NULL)

lxclipboard_lxclipboard_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg gtk+-3.0 \
	--pkg lxclipboard \
	--define USE_GTK3 \
	$(NULL)

lxclipboard_lxclipboard_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

lxclipboard_lxclipboard_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# Polkit Authorization Agent
lxpolkit_lxpolkit_SOURCES = \
	lxpolkit/main.vala \
	lxpolkit/lxpolkit.c \
	lxpolkit/lxpolkit-listener.c \
	$(NULL)

lxpolkit_lxpolkit_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg gtk+-3.0 \
	--pkg lxpolkit \
	--define USE_GTK3 \
	$(NULL)

lxpolkit_lxpolkit_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(POLKIT_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)"\" \
	-DPACKAGE_UI_DIR="\"$(datadir)/lxsession/ui"\" \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

lxpolkit_lxpolkit_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(POLKIT_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# Session Database Engine
lxsession_db_lxsession_db_SOURCES = \
	lxsession-db/main.vala \
	lxsession-db/desktop-files-backend.vala \
	$(NULL)

lxsession_db_lxsession_db_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg gtk+-3.0 \
	$(NULL)

lxsession_db_lxsession_db_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)"\" \
	-DPACKAGE_UI_DIR="\"$(datadir)/lxsession-default-apps/ui"\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

lxsession_db_lxsession_db_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# Session Editor Panel
lxsession_edit_lxsession_edit_SOURCES = \
	lxsession-edit/lxsession-edit.c \
	lxsession-edit/lxsession-edit-common.c \
	$(NULL)

lxsession_edit_lxsession_edit_CPPFLAGS = \
	-I$(top_srcdir) \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)"\" \
	-DPACKAGE_UI_DIR="\"$(datadir)/lxsession/ui"\" \
	-Werror=implicit-function-declaration \
	$(NULL)

# Modern Fix: Added critical GLIB and X11 target link suites to block link breakdown drops.
lxsession_edit_lxsession_edit_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(X11_LIBS) \
	$(INTLLIBS) \
	$(NULL)

# Default Application Configurator
lxsession_default_apps_lxsession_default_apps_SOURCES = \
	lxsession-default-apps/autostart.vala \
	lxsession-default-apps/main.vala \
	lxsession-default-apps/combobox.vala \
	lxsession-default-apps/dbus-backend.vala \
	lxsession-default-apps/utils.vala \
	lxsession-edit/lxsession-edit-common.c \
	$(NULL)

lxsession_default_apps_lxsession_default_apps_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg gtk+-3.0 \
	--pkg gio-2.0 \
	--pkg posix \
	--pkg lxsession-edit \
	--pkg config \
	--define USE_GTK3 \
	$(NULL)

# Modern Fix: Purged obsolete non-existent DBUSGLIB variables to sync with the clean configure layer.
lxsession_default_apps_lxsession_default_apps_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)"\" \
	-DPACKAGE_UI_DIR="\"$(datadir)/lxsession-default-apps/ui"\" \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

lxsession_default_apps_lxsession_default_apps_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# Clipboard Builtins
if USE_BUILDIN_CLIPBOARD
buildinclip_SOURCES = lxclipboard/clipboard.c
buildinclip_VALAFLAGS = --pkg lxclipboard -D BUILDIN_CLIPBOARD
endif

# Polkit Builtins
if USE_BUILDIN_POLKIT
buildinpolkit_SOURCES = \
	lxpolkit/lxpolkit.c \
	lxpolkit/lxpolkit-listener.c
buildinpolkit_VALAFLAGS = --pkg lxpolkit -D BUILDIN_POLKIT
buildinpolkit_CFLAGS = $(POLKIT_CFLAGS)
buildinpolkit_LDADD = $(POLKIT_LIBS)
endif

# Logout Dialog
lxsession_logout_lxsession_logout_SOURCES = \
	lxsession-logout/lxsession-logout-dbus-interface.c \
	lxsession-logout/lxsession-logout.c \
	$(NULL)

lxsession_logout_lxsession_logout_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)"\" \
	-Werror=implicit-function-declaration \
	$(NULL)

lxsession_logout_lxsession_logout_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# XSettings Daemon Core
lxsettings_daemon_lxsettings_daemon_commun_SOURCES = \
	lxsettings-daemon/settings-daemon.c \
	lxsettings-daemon/xevent.c \
	lxsettings-daemon/xsettings-common.c \
	lxsettings-daemon/xsettings-manager.c \
	lxsettings-daemon/xutils.c \
	$(NULL)

lxsettings_daemon_lxsettings_daemon_SOURCES = \
	lxsettings-daemon/main.vala \
	$(lxsettings_daemon_lxsettings_daemon_commun_SOURCES) \
	$(NULL)

lxsettings_daemon_lxsettings_daemon_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg lxsettings-daemon \
	$(NULL)

lxsettings_daemon_lxsettings_daemon_CPPFLAGS = \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

lxsettings_daemon_lxsettings_daemon_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# Autostart Agent
xdg_autostart_lxsession_xdg_autostart_SOURCES = \
	xdg-autostart/main.vala \
	xdg-autostart/xdg-autostart.c \
	$(NULL)

xdg_autostart_lxsession_xdg_autostart_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg xdg-autostart \
	$(NULL)

xdg_autostart_lxsession_xdg_autostart_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

xdg_autostart_lxsession_xdg_autostart_LDADD = \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(X11_LIBS) \
	$(NULL)

# Main Session Manager
lxsession_lxsession_SOURCES = \
	lxsession/main.vala \
	lxsession/app.vala \
	lxsession/autostart.vala \
	lxsession/conffiles.vala \
	lxsession/control.vala \
	lxsession/dbus-common.vala \
	lxsession/dbus-gnome-session.vala \
	lxsession/dbus-lxde-session.vala \
	lxsession/environement.vala \
	lxsession/notifications.vala \
	lxsession/options.vala \
	lxsession/process.vala \
	lxsession/settings.vala \
	lxsession/utils.vala \
	$(lxsettings_daemon_lxsettings_daemon_commun_SOURCES) \
	$(buildinclip_SOURCES) \
	$(buildinpolkit_SOURCES) \
	$(NULL)

lxsession_lxsession_VALAFLAGS = \
	--vapidir=$(srcdir)/vapi \
	--pkg gio-2.0 \
	--pkg posix \
	--pkg lxsettings-daemon \
	--pkg gtk+-3.0 \
	-D USE_GTK \
	$(buildinclip_VALAFLAGS) \
	$(buildinpolkit_VALAFLAGS) \
	$(NULL)

# Modern Fix: Cleared out all dead dbus-glib-1 header flags and aligned layouts safely.
lxsession_lxsession_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_DATA_DIR="\"$(datadir)"\" \
	-DPACKAGE_UI_DIR="\"$(datadir)/lxsession/ui"\" \
	-I$(srcdir)/lxsettings-daemon \
	-I$(srcdir)/xdg-autostart \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GIO_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	$(buildinclip_CFLAGS) \
	$(buildinpolkit_CFLAGS) \
	-include config.h \
	-Werror=implicit-function-declaration \
	$(NULL)

lxsession_lxsession_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(GIO_LIBS) \
	$(X11_LIBS) \
	$(buildinclip_LDADD) \
	$(buildinpolkit_LDADD) \
	$(NULL)

if USE_ADVANCED_NOTIFICATIONS
lxsession_lxsession_VALAFLAGS += \
	--pkg appindicator-0.1 \
	--pkg libnotify \
	-D USE_ADVANCED_NOTIFICATIONS

lxsession_lxsession_CPPFLAGS += \
	$(LIBNOTIFY_CFLAGS) \
	$(INDICATORS_CFLAGS)

lxsession_lxsession_LDADD += \
	$(LIBNOTIFY_LIBS) \
	$(INDICATORS_LIBS)
endif