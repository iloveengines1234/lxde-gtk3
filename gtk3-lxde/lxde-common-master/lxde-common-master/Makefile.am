# Modernized root level Makefile.am for lx-common-master
# Fixes parallel installation bugs and cleans syntax errors.

NULL =

SUBDIRS = man po

bin_SCRIPTS = startlxde lxde-logout openbox-lxde

# Target XSession environment settings
xsessiondir = $(xsesspath)
xsession_DATA = LXDE.desktop

# Default core desktop environments profile data
defaultsdir = $(sysconfdir)/xdg/lxsession/LXDE
defaults_DATA = \
	autostart \
	desktop.conf \
	$(NULL)

# Applications and Desktop entries handling via clean Intltool rules
desktopdir = $(datadir)/applications
desktop_in_files = \
	lxde-logout.desktop.in \
	lxde-screenlock.desktop.in \
	$(NULL)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

# Local desktop assets definitions
wallpapersdir = $(datadir)/lxde/wallpapers
wallpapers_DATA = \
	wallpapers/lxde_blue.jpg \
	wallpapers/lxde_red.jpg \
	wallpapers/lxde_green.jpg \
	$(NULL)

imagesdir = $(datadir)/lxde/images
images_DATA = \
	images/logout-banner.png \
	images/lxde-icon.png \
	$(NULL)

# LXPanel configuration directory layouts
lxpaneldir = $(sysconfdir)/xdg/lxpanel/LXDE
lxpanel_DATA = lxpanel/config

lxpanel_paneldir = $(sysconfdir)/xdg/lxpanel/LXDE/panels
lxpanel_panel_DATA = lxpanel/panel

# Window manager configurations
openboxdir = $(sysconfdir)/xdg/openbox/LXDE
openbox_DATA = \
	openbox/rc.xml \
	openbox/menu.xml \
	$(NULL)

# File manager baseline application defaults
pcmanfmdir = $(sysconfdir)/xdg/pcmanfm/LXDE
pcmanfm_DATA = \
	pcmanfm/pcmanfm.conf \
	$(NULL)

# Re-architected distribution manifest tracking list
EXTRA_DIST = \
	autostart \
	desktop.conf \
	LXDE.desktop.in \
	$(desktop_in_files) \
	startlxde.in \
	$(wallpapers_DATA) \
	$(images_DATA) \
	openbox/rc.xml.in \
	openbox/menu.xml \
	openbox-lxde.in \
	pcmanfm/pcmanfm.conf.in \
	$(lxpanel_DATA) \
	lxpanel/panel.in \
	$(NULL)

# Ensure all intermediate generated assets are cleaned out cleanly
CLEANFILES = \
	$(bin_SCRIPTS) \
	LXDE.desktop \
	$(desktop_DATA) \
	$(NULL)