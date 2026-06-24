## Process this file with automake to produce Makefile.in

# Preprocessor flags
AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-Wall \
	-Werror-implicit-function-declaration \
	-DMENUCACHE_LIBEXECDIR="\"$(pkglibexecdir)\"" \
	-DG_LOG_DOMAIN=\"Menu-Cache\"

# Library definitions
lib_LTLIBRARIES = libmenu-cache.la

libmenu_cache_la_SOURCES = \
	menu-cache.c

libmenu_cache_la_LIBADD = \
	$(GLIB_LIBS)

libmenu_cache_la_LDFLAGS = \
	-no-undefined \
	-export-symbols-regex menu_cache \
	-version-info 5:1:2

# Header installation routing
lib_menu_cache_includedir = $(includedir)/menu-cache
nodist_lib_menu_cache_include_HEADERS = \
	menu-cache.h

# Extra distribution targets tracking
EXTRA_DIST = \
	version.h \
	libmenu-cache.pc.in

# Pkg-config configuration definitions
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmenu-cache.pc