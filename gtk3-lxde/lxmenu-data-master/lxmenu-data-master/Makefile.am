## Process this file with automake to produce Makefile.in

NULL =

# --- Distribution Subdirectories Loop ---
# Natively processes structural layouts before building out localization binaries
SUBDIRS = \
	layout \
	desktop-directories \
	po \
	$(NULL)

# --- Configuration Macro Directives ---
# Natively informs aclocal where to look up your modular package dependencies
ACLOCAL_AMFLAGS = -I m4

# --- Distribution Package Source Manifest ---
# Expanded to explicitly pack local build environment validation scripts and macro roots
EXTRA_DIST = \
	autogen.sh \
	config.rpath \
	m4/gnome-doc-utils.m4 \
	$(NULL)

# --- Extended Deep Cleanup Tracking Blocks ---
# Ensures that all intltool transient execution scripts and template caches are fully wiped
DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(NULL)