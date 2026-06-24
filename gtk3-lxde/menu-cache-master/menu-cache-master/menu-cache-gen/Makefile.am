## Process this file with automake to produce Makefile.in

# Preprocessor flags and internal dependency header search paths
AM_CPPFLAGS = \
	-I$(top_srcdir)/libmenu-cache \
	-I$(top_builddir)/libmenu-cache \
	$(GLIB_CFLAGS) \
	$(LIBFM_EXTRA_CFLAGS) \
	$(DEBUG_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-Wall \
	-Werror-implicit-function-declaration

# Generator application installation path definition
pkglibexec_PROGRAMS = menu-cache-gen

# Source files parsing layout
menu_cache_gen_SOURCES = \
	main.c \
	menu-merge.c \
	menu-compose.c

# Linker library tracking flags
menu_cache_gen_LDADD = \
	$(GLIB_LIBS) \
	$(LIBFM_EXTRA_LIBS)

# Non-compiling internal headers for inclusion in distribution payloads
EXTRA_DIST = \
	menu-tags.h