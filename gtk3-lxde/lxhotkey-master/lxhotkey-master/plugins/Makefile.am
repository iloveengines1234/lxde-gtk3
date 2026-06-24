## Process this file with automake to produce Makefile.in

# Common compiler preprocessor search directives
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	$(PACKAGE_CFLAGS)

# Common runtime execution switches for modular libraries
# MODERN FIX: Extracted dynamic library link tokens out to their explicit target _LIBADD variables
AM_LDFLAGS = \
	-module \
	-avoid-version \
	-shared \
	-export-dynamic \
	-no-undefined \
	-rpath $(libdir)/lxhotkey

# Target installation directory for modular extensions
pkglibdir = $(libdir)/lxhotkey

# Initialize baseline collection blocks
pkglib_LTLIBRARIES = ob.la
if WITH_GTK
pkglib_LTLIBRARIES += gtk.la
endif

# --- Openbox Backend Window Manager Plugin Module ---
ob_la_SOURCES = openbox/openbox.c
ob_la_LIBADD = $(PACKAGE_LIBS) -lfm-extra

# --- Graphical Frontend Toolkit Plugin Module ---
gtk_la_SOURCES = gtk/gtk.c gtk/edit.c
gtk_la_CPPFLAGS = $(AM_CPPFLAGS) $(GTK_CFLAGS)
gtk_la_LIBADD = $(PACKAGE_LIBS) $(GTK_LIBS)

# --- Internationalized Desktop Shortcut Assets Integration ---
desktopdir = $(datadir)/applications
desktop_in_files =
if WITH_GTK
desktop_in_files += gtk/lxhotkey-gtk.desktop.in
endif

desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

# Auxiliary configuration files distributed inside release archives
EXTRA_DIST = \
	gtk/lxhotkey-gtk.desktop.in \
	gtk/edit.h

DISTCLEANFILES = \
	gtk/lxhotkey-gtk.desktop

# MODERN FIX: Intercept the uninstallation lookup safely before 
# clearing standard metadata blocks out of the environment target paths.
uninstall-hook:
	rm -f $(DESTDIR)$(pkglibdir)/ob.so
	if test -f $(DESTDIR)$(pkglibdir)/gtk.so; then \
		rm -f $(DESTDIR)$(pkglibdir)/gtk.so; \
	fi

install-exec-hook:
	rm -f $(DESTDIR)$(pkglibdir)/*.la