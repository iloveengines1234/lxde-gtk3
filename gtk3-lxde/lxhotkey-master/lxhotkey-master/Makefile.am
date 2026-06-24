## Process this file with automake to produce Makefile.in
# Makefile.am for lxhotkey

NULL =

# --- Global Compiler Macro Routing Architecture ---
# Tells aclocal where to look up custom infrastructure and packaging dependencies
ACLOCAL_AMFLAGS = -I m4

# --- High-Level Build Directory Iteration Targets ---
SUBDIRS = src plugins po

# --- Pkg-config Specification Generation Pathways ---
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = lxhotkey.pc

# --- System Manual Page Distributions ---
man_MANS = man/lxhotkey.1

# --- Distribution Package Source Manifest ---
# CRITICAL FIX: Expanded to explicitly include the m4 folder macro definitions,
# preventing missing-dependency failures during out-of-tree builds.
EXTRA_DIST = \
	autogen.sh \
	lxhotkey.pc.in \
	man/lxhotkey.1.in \
	m4/gnome-doc-utils.m4 \
	$(NULL)

# --- Extended Deep Cleanup Tracking Blocks ---
# Natively ensures that all transient configuration and manual artifacts are wiped
DISTCLEANFILES = \
	lxhotkey.pc \
	man/lxhotkey.1 \
	$(NULL)