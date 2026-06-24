ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src po man

# Modern Fix: Compile the UI layout natively into the binary application structure 
# via GResource rules instead of distributing loose layout XML fragments to disk.
glib_resources_xml = src/lxinput-resources.gresource.xml
glib_resources_c = src/lxinput-resources.c

desktopdir = $(datadir)/applications
desktop_in_files = data/lxinput.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Modernized translation merging rule sequence
@INTLTOOL_DESKTOP_RULE@

iconsdir = $(datadir)/lxinput
icons_DATA = \
	data/input-keyboard.png \
	data/input-mouse.png

# Crucial fix: Track the XML blueprint, raw layout assets, and desktop templates safely inside the archive
EXTRA_DIST = \
	$(glib_resources_xml) \
	data/lxinput.ui \
	$(desktop_in_files) \
	$(icons_DATA)

# Clean up generated desktop files, GResource implementations, and translation caching structures during a reset
CLEANFILES = \
	$(desktop_DATA) \
	$(glib_resources_c) \
	po/.intltool-merge-cache