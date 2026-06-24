# Define explicit structural subdirectories
SUBDIRS = ui

# Core desktop entry installations
desktopdir = $(datadir)/applications
desktop_in_files = \
	lxsession-default-apps.desktop.in \
	lxsession-edit.desktop.in

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Autostart fallback triggers
autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = lxpolkit.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

# Hook the localized intltool generation engine rules
@INTLTOOL_DESKTOP_RULE@

# Session resource imagery assets
imagedir = $(datadir)/lxsession/images
image_DATA = \
	images/system-shutdown.png \
	images/system-log-out.png \
	images/gnome-session-hibernate.png \
	images/gnome-session-reboot.png \
	images/gnome-session-suspend.png \
	images/gnome-session-switch.png

# Crucial Fix: Bundle ONLY static template sources into release tarballs
EXTRA_DIST = \
	desktop.conf.example \
	conffiles.conf.example \
	$(desktop_in_files) \
	$(autostart_in_files) \
	$(image_DATA)

# Crucial Fix: Track dynamically generated assets under target compile queues
CLEANFILES = \
	$(desktop_DATA) \
	$(autostart_DATA)