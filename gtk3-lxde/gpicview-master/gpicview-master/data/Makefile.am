## Process this file with automake to produce Makefile.in

# Global data directory definition
gpicviewdir = $(datadir)/gpicview

# Pixmaps installation configuration
pixmapsdir = $(gpicviewdir)/pixmaps
pixmaps_DATA = \
	pixmaps/object-rotate-right.png \
	pixmaps/object-rotate-left.png \
	pixmaps/object-flip-horizontal.png \
	pixmaps/object-flip-vertical.png

# User interface definition files (GtkBuilder / XML layouts)
uidir = $(gpicviewdir)/ui
ui_DATA = \
	pref-dlg.ui

# Manual pages distribution
dist_man_MANS = gpicview.1

# Explicitly list additional asset payloads for the distribution tarball
EXTRA_DIST = \
	$(ui_DATA) \
	$(pixmaps_DATA) \
	$(dist_man_MANS)