# ==============================================================================
# Makefile.am - GNU Automake Template File
# Component: lxappearance-obconf
# ==============================================================================

# Ensure aclocal flags locate local macro directories correctly
ACLOCAL_AMFLAGS = -I m4

# Define build subdirectories loop targets
SUBDIRS = po

# Standard data layout hierarchies configuration
localedir        = $(datadir)/locale
gladedir         = $(LXAPPEARANCE_DATADIR)/obconf
pixmapdir        = $(datadir)/pixmaps
themedir         = $(datadir)/openbox/themes
desktopdir       = $(datadir)/applications

# Establish destination directory for the plugin binary shared module
moduledir        = $(LXAPPEARANCE_LIBDIR)
module_LTLIBRARIES = src/obconf.la

# Compilation preprocessor flag macros (Optimized for GTK 3.24.52+)
src_obconf_la_CPPFLAGS = \
	$(OPENBOX_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LXAPPEARANCE_CFLAGS) \
	$(GMODULE_CFLAGS) \
	$(GDK_PIXBUF_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-DLOCALEDIR=\"$(localedir)\" \
	-DPIXMAPDIR=\"$(pixmapdir)\" \
	-DGLADEDIR=\"$(gladedir)\" \
	-DTHEMEDIR=\"$(themedir)\" \
	-DG_LOG_DOMAIN=\"Obconf\"

# Shared library dependencies linking specifications
src_obconf_la_LIBADD = \
	$(OPENBOX_LIBS) \
	$(GTK_LIBS) \
	$(LXAPPEARANCE_LIBS) \
	$(GMODULE_LIBS) \
	$(GDK_PIXBUF_LIBS) \
	$(LIBINTL)

# Fixed linker profile parameters to output a clean loadable module runtime
src_obconf_la_LDFLAGS = \
	-module \
	-avoid-version \
	-shared \
	$(LXAPPEARANCE_MODULE)

# Source code files array mapping
src_obconf_la_SOURCES = \
	src/archive.c \
	src/main.c \
	src/appearance.c \
	src/preview.c \
	src/preview_update.c \
	src/theme.c \
	src/tree.c

# Cleanly isolated headers for precise target tracking management
noinst_HEADERS = \
	src/archive.h \
	src/main.h \
	src/appearance.h \
	src/preview.h \
	src/preview_update.h \
	src/theme.h \
	src/tree.h

# Graphical user interface configuration map data files tracking
glade_DATA = src/obconf.glade

# Miscellaneous development distribution assets tracking loop arrays
EXTRA_DIST = \
	autogen.sh \
	README \
	CHANGELOG \
	COPYING \
	AUTHORS \
	src/obconf.glade

# Maintenance hooks for system database synchronization
install-data-hook:
	@if [ -z "$(DESTDIR)" ] && command -v update-desktop-database >/dev/null 2>&1; then \
		echo "Updating Desktop application launcher database..."; \
		update-desktop-database "$(desktopdir)" || true; \
	fi

uninstall-hook:
	@if [ -z "$(DESTDIR)" ] && command -v update-desktop-database >/dev/null 2>&1; then \
		echo "Updating Desktop application launcher database..."; \
		update-desktop-database "$(desktopdir)" || true; \
	fi