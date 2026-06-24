default_config_DATA = lxterminal.conf
default_configdir = $(datadir)/lxterminal

uidir = $(datadir)/lxterminal
ui_DATA = lxterminal-preferences.ui menu.ui

lxterminal.desktop_in_files = lxterminal.desktop.in
lxterminal.desktop.in_DATA = $(lxterminal.desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
lxterminal.desktop.indir = $(datadir)/applications

lxterminal_images_DATA = lxterminal.png
lxterminal_imagesdir = $(datadir)/icons/hicolor/128x128/apps

EXTRA_DIST = \
	lxterminal.conf.in \
	$(lxterminal_images_DATA) \
	$(lxterminal.desktop_in_files) \
	$(lxterminal.desktop.in_DATA) \
	$(ui_DATA) \
	$(default_config_DATA)