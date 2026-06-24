## Process this file with automake to produce Makefile.in

# --- Global Preprocessor Core Flags ---
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)/lxlauncher\"" \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)\"" \
	$(GTK3_CFLAGS) \
	$(MENU_CFLAGS) \
	$(ADDITIONAL_FLAGS)

# --- Target Binary Target Assignments ---
bin_PROGRAMS = lxlauncher

# --- Source Manifest Distribution Mapping ---
lxlauncher_SOURCES = \
	lxlauncher.c \
	misc.c misc.h \
	glib-mem.h \
	vfs-utils.c vfs-utils.h \
	vfs-execute.c vfs-execute.h \
	working-area.c working-area.h \
	exo-wrap-table.c exo-wrap-table.h

# --- Linker Shared Object Architecture Bindings ---
lxlauncher_LDADD = \
	$(GTK3_LIBS) \
	$(MENU_LIBS) \
	$(INTLLIBS)

# --- Clean Optional Execution Modifiers ---
lxlauncher_LDFLAGS = -no-undefined