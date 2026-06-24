## Process this file with automake to produce Makefile.in

# Preprocessor flags and internal dependency header search paths
AM_CPPFLAGS = \
	-I$(top_srcdir)/libmenu-cache \
	-I$(top_builddir)/libmenu-cache \
	$(GLIB_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-DMENUCACHE_LIBEXECDIR="\"$(pkglibexecdir)\"" \
	-Wall \
	-Werror-implicit-function-declaration

# Daemon program installation path definition
pkglibexec_PROGRAMS = menu-cached

# Source files mapping
menu_cached_SOURCES = \
	menu-cached.c

# Linker library tracking flags
menu_cached_LDADD = \
	$(GLIB_LIBS)

# Linker behavior settings
menu_cached_LDFLAGS = \
	-no-undefined