## Makefile.am -- Process this file with automake to produce Makefile.in

NULL =

# Core compilation flags updated to globally support the GTK3 setup
AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/base -I$(top_srcdir)/src/job \
	$(GIO_CFLAGS) \
	$(GTK_CFLAGS)

# Linker flags updated to globally bridge GTK objects cleanly inside WSL
AM_LDFLAGS = \
	-no-undefined -module -avoid-version -shared -export-dynamic \
	-L$(libdir) \
	$(GIO_LIBS) \
	$(GTK_LIBS)

# The module should be linked only with libfm, not libfm-gtk or whatever
LIBS += $(top_builddir)/src/libfm.la

# Change default and put modules below usual place
pkglibdir = $(libdir)/@PACKAGE@/modules

# Modules to compile
pkglib_LTLIBRARIES = \
	vfs-menu.la \
	vfs-search.la \
	$(NULL)

if ENABLE_GTK
pkglib_LTLIBRARIES += \
	gtk-fileprop-x-desktop.la \
	gtk-fileprop-x-shortcut.la \
	gtk-menu-trash.la \
	gtk-menu-actions.la \
	$(NULL)
endif

# Install only .so files, we don't need .la files
install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.la

PLUGINS_INSTALLED = $(pkglib_LTLIBRARIES:.la=.so)
uninstall-hook:
	cd $(DESTDIR)$(pkglibdir) && rm -f $(PLUGINS_INSTALLED) || true

# Module-specific source and parameters mapping
vfs_menu_la_SOURCES = vfs-menu.c
vfs_menu_la_CFLAGS = $(MENU_CACHE_CFLAGS) -I$(top_srcdir)/src/extra
vfs_menu_la_LIBADD = $(MENU_CACHE_LIBS) $(top_builddir)/src/libfm-extra.la

vfs_search_la_SOURCES = vfs-search.c

gtk_fileprop_x_desktop_la_SOURCES = gtk-fileprop-x-desktop.c
gtk_fileprop_x_desktop_la_CFLAGS = $(GTK_CFLAGS) -I$(top_srcdir)/src/gtk
gtk_fileprop_x_desktop_la_LIBADD = $(GTK_LIBS)

gtk_fileprop_x_shortcut_la_SOURCES = gtk-fileprop-x-shortcut.c
gtk_fileprop_x_shortcut_la_CFLAGS = $(GTK_CFLAGS) -I$(top_srcdir)/src/gtk
gtk_fileprop_x_shortcut_la_LIBADD = $(GTK_LIBS)

gtk_menu_actions_la_SOURCES = gtk-menu-actions.c
gtk_menu_actions_la_CFLAGS = -I$(top_srcdir)/src/actions $(GTK_CFLAGS) -I$(top_srcdir)/src/gtk
gtk_menu_actions_la_LIBADD = $(GTK_LIBS) $(top_builddir)/src/@LIBFM_GTK_LTLIBRARIES@

gtk_menu_trash_la_SOURCES = gtk-menu-trash.c
gtk_menu_trash_la_CFLAGS = $(GTK_CFLAGS) -I$(top_srcdir)/src/gtk
gtk_menu_trash_la_LIBADD = $(GTK_LIBS) $(top_builddir)/src/@LIBFM_GTK_LTLIBRARIES@