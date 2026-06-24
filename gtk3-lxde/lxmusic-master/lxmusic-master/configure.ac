dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lxmusic], [0.4.8], [http://lxde.org/])
AC_CONFIG_SRCDIR([src/lxmusic.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz serial-tests])
AM_MAINTAINER_MODE([enable])

dnl Support silent build rules. Disable by passing --disable-silent-rules
AM_SILENT_RULES([yes])

# Minimum Package Versions
m4_define([gtk_minimum_version], [3.24.52])
m4_define([gmodule_export_minimum_version], [2.0])
m4_define([xmms2_minimum_version], [0.7])
m4_define([xmms2_client_glib_minimum_version], [0.7])
m4_define([libnotify_minimum_version], [0.4.1])

# Checks for programs.
AC_PROG_CC

# Setup internationalization
IT_PROG_INTLTOOL([0.40.0], [no-xml])

# Checks for mandatory modern GTK 3 modules
PKG_CHECK_MODULES([LXMUSIC], [
    gtk+-3.0 >= gtk_minimum_version
    gmodule-export-2.0 >= gmodule_export_minimum_version
    xmms2-client >= xmms2_minimum_version
    xmms2-client-glib >= xmms2_client_glib_minimum_version
])

# Checks for optional libnotify module
PKG_CHECK_MODULES([LIBNOTIFY], [libnotify >= libnotify_minimum_version], [
    HAVE_LIBNOTIFY="yes"
    AC_DEFINE([HAVE_LIBNOTIFY], [1], [Define to 1 if libnotify is available])
], [
    HAVE_LIBNOTIFY="no"
])
AM_CONDITIONAL([HAVE_LIBNOTIFY], [test "x$HAVE_LIBNOTIFY" = "xyes"])

# Localization settings
GETTEXT_PACKAGE=lxmusic
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package.])

# Dynamically generate po/LINGUAS
sys_po_dir="$srcdir/po"
rm -f "$sys_po_dir/LINGUAS"
if test -d "$sys_po_dir"; then
    for po_file in $(ls "$sys_po_dir"/*.po 2>/dev/null | sort); do
        if test -f "$po_file"; then
            lang=$(basename "$po_file" .po)
            echo "$lang" >> "$sys_po_dir/LINGUAS"
        fi
    done
fi

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

AC_CONFIG_FILES([
    Makefile
    data/Makefile
    src/Makefile
    po/Makefile.in
])
AC_OUTPUT