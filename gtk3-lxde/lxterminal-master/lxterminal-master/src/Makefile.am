## Process this file with automake to produce Makefile.in

bin_PROGRAMS = lxterminal

# Preprocessor definitions (Strictly limited to includes and macros)
lxterminal_CPPFLAGS = \
	-I$(top_srcdir) \
	-DPACKAGE_BIN_DIR=\""$(bindir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\"

# Compiler adjustment rules flags
lxterminal_CFLAGS = \
	$(PACKAGE_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	$(G_CAST_CHECKS)

# Explicit target compilation unit tracking allocations
lxterminal_SOURCES = \
	unixsocket.h \
	unixsocket.c \
	setting.h \
	setting.c \
	lxterminal.h \
	lxterminal.c \
	preferences.h \
	preferences.c

# Linker resolution options mapping
lxterminal_LDADD = \
	$(PACKAGE_LIBS) \
	$(INTLLIBS)

# Non-compilable assets to include in standard distribution tarballs
EXTRA_DIST = \
	README.md