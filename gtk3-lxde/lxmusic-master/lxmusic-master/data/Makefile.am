# Desktop file generation
desktop_in_files = lxmusic.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
desktopdir = $(datadir)/applications

@INTLTOOL_DESKTOP_RULE@

# Icons
icon_DATA = lxmusic.png
icondir = $(datadir)/icons/hicolor/128x128/apps

# Man pages
man_MANS = lxmusic.1

# GtkBuilder UI definitions (Updated extensions for GTK3)
uidir = $(datadir)/lxmusic
ui_DATA = \
	lxmusic.ui \
	pref-dlg.ui \
	track-info.ui

# Files to distribute in the tarball
EXTRA_DIST = \
	$(desktop_in_files) \
	$(icon_DATA) \
	$(ui_DATA) \
	$(man_MANS)

# Clean generated files locally
CLEANFILES = \
	$(desktop_DATA)