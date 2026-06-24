# Process this file with automake to produce Makefile.in
# Updated by iloveengines1234 to port completely to GTK 3.24.52 / Modern Autotools

NULL =

# Core execution target
bin_PROGRAMS = lxrandr

# Explicit source tracking
lxrandr_SOURCES = \
	lxrandr.c \
	$(NULL)

# Modern Fix: Ensure the locale directory paths are strictly mapped to standard 
# GNU installation variables ($(localedir)). Added explicitly isolated macro tracking flags.
lxrandr_CFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-include config.h \
	$(GTK_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	$(NULL)

# Modern Fix: Added critical GLIB, X11, and core platform target link suites 
# to block transitive linker drops on modern linux distributions.
lxrandr_LDADD = \
	$(GTK_LIBS) \
	$(GLIB_LIBS) \
	$(X11_LIBS) \
	$(INTLLIBS) \
	$(NULL)