## Process this file with automake to produce Makefile.in

# Core binary execution target identifier
bin_PROGRAMS = lxhotkey

# Global preprocessor compilation include search paths and definitions
lxhotkey_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_PLUGINS_DIR=\""$(libdir)/lxhotkey"\" \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	$(PACKAGE_CFLAGS) \
	$(GTK_CFLAGS)

# Primary engine translation source file units
lxhotkey_SOURCES = lxhotkey.c

# Linker module configuration properties
# MODERN FIX: Added -export-dynamic so modular backend plugins can safely 
# reference definitions hosted back inside the core software engine context.
lxhotkey_LDFLAGS = -export-dynamic

# External dependent dynamic link library archives
lxhotkey_LDADD = \
	$(PACKAGE_LIBS) \
	$(GTK_LIBS)

# Public library development header locations
lxhotkey_includedir = $(includedir)/lxhotkey
lxhotkey_include_HEADERS = lxhotkey.h

# Clean up residual system assembly file artifacts on demand
CLEANFILES = $(bin_PROGRAMS)