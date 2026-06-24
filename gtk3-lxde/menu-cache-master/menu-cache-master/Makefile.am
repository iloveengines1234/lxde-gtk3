## Process this file with automake to produce Makefile.in

# Local macro directory tracking for aclocal
ACLOCAL_AMFLAGS = -I m4

# Document subdirectories definition
SUBDIRS_DOCS = docs

# Document distribution files layout tracking
EXTRA_DIST_DOCS = \
	gtk-doc.make \
	docs/Makefile.am \
	docs/Makefile.in \
	docs/reference/Makefile.am \
	docs/reference/Makefile.in \
	docs/reference/libmenu-cache/Makefile.am \
	docs/reference/libmenu-cache/Makefile.in

# Define foundational library and application subdirectories
ALL_SUBDIRS = \
	libmenu-cache \
	menu-cache-gen \
	menu-cache-daemon

# Standard target directories definitions
SUBDIRS = $(ALL_SUBDIRS)
DIST_SUBDIRS = $(ALL_SUBDIRS)

# Catch-all distribution payload tracking variable
EXTRA_DIST = 

# Conditional tracking rules for api documentation builds
if ENABLE_GTK_DOC
DIST_SUBDIRS += $(SUBDIRS_DOCS)
SUBDIRS += $(SUBDIRS_DOCS)
else
EXTRA_DIST += $(EXTRA_DIST_DOCS)
endif

# Configuration validation flag settings for distcheck
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc