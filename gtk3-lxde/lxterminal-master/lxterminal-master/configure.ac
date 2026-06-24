# configure.ac - Configuration script template for lxterminal
#
# Copyright (C) 2016-2021 Andriy Grytsenko <andrej@rep.kiev.ua>
# Copyright (C) 2026 LXDE Project Maintainers
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

AC_PREREQ([2.69])
AC_INIT([lxterminal], [0.4.1], [http://lxde.org/])
AC_CONFIG_SRCDIR([src/lxterminal.c])
AC_CONFIG_HEADERS([config.h])

# Support silent build rules. Disable by either passing --disable-silent-rules
# to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdepth-objects])

# Initialize processing translation modules
AC_PROG_INTLTOOL([0.40.0])
AM_MAINTAINER_MODE([enable])

# Check for primary compiler environments
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Verify core system target header locations
AC_CHECK_HEADERS([locale.h langinfo.h sys/stat.h sys/types.h])

# Build Configuration Options
AC_ARG_ENABLE([man],
    [AS_HELP_STRING([--enable-man],
                    [regenerate roff man pages from Docbook @<:@default=no@:>@])],
    [enable_man="${enableval}"],
    [enable_man=no]
)

AC_ARG_ENABLE([more-warnings],
    [AS_HELP_STRING([--enable-more-warnings],
                    [Add explicit strict warning compilation profiles @<:@default=no@:>@])],
    [enable_more_warnings="${enableval}"],
    [enable_more_warnings=no]
)

AC_ARG_ENABLE([gtk3],
    [AS_HELP_STRING([--enable-gtk3],
                    [enable to use gtk-3.0 instead of gtk-2.0 @<:@default=no@:>@])],
    [case "${enableval}" in
       yes) enable_gtk3=yes ;;
       no)  enable_gtk3=no ;;
       *)   AC_MSG_ERROR([bad value "${enableval}" for --enable-gtk3, use "yes" or "no".]) ;;
     esac],
    [enable_gtk3=no]
)

# Core Library Infrastructure Verification Checks
PKG_CHECK_MODULES([X11], [x11])
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.16.0])

if test "x$enable_gtk3" = "xyes"; then
    AC_DEFINE([ENABLE_GTK3], [1], [Enable compilation targeting GTK+3 UI kits])
    gtk_modules="gtk+-3.0 >= 3.0.0"
else
    gtk_modules="gtk+-2.0 >= 2.18.0"
fi
PKG_CHECK_MODULES([GTK], [$gtk_modules])

# Perform validation tests on underlying virtual terminal emulation engines
PKG_CHECK_EXISTS([vte-2.91 >= 0.46.0], [
    AC_CHECK_HEADER([pcre2.h], [], [
        AC_MSG_ERROR([required libpcre2 header file pcre2.h not found])
    ], [#define PCRE2_CODE_UNIT_WIDTH 0])
])

if test "x$enable_gtk3" = "xyes"; then
    PKG_CHECK_MODULES([VTE], [vte-2.91 >= 0.38.0], [], [
        PKG_CHECK_MODULES([VTE], [vte-2.90 >= 0.28.0])
    ])
else
    PKG_CHECK_MODULES([VTE], [vte >= 0.26.0])
fi

# Consolidate package flags cleanly without rewriting default environment variables
ADDITIONAL_FLAGS=""
if test "x$enable_more_warnings" = "xyes"; then
    ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration \
-Werror=implicit-int -Werror=missing-braces -Werror=parentheses -Werror=return-type \
-Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value \
-Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations \
-Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline \
-Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self \
-Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute \
-Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2 \
-DG_DISABLE_DEPRECATED -DG_DISABLE_SINGLE_INCLUDES -DGDK_DISABLE_DEPRECATED \
-DGDK_PIXBUF_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_DEPRECATED \
-DGTK_DISABLE_SINGLE_INCLUDES"
fi
AC_SUBST([ADDITIONAL_FLAGS])

# Configure dynamic target documentation generation wrappers
if test "x$enable_man" = "xyes"; then
    AC_PATH_PROG([XSLTPROC], [xsltproc])
    if test -z "$XSLTPROC"; then
        AC_MSG_ERROR([xsltproc is required to regenerate the pre-built man page; consider --disable-man])
    fi

    dnl Check for appropriate DocBook components inside localized system tables
    JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
        [DocBook XML DTD V4.1.2], [], [AC_MSG_ERROR([DocBook XML DTD is required to regenerate the pre-built man page; consider --disable-man])])
    JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
        [DocBook XSL Stylesheets >= 1.70.1], [], [AC_MSG_ERROR([DocBook XSL Stylesheets are required to regenerate the pre-built man page; consider --disable-man])])
fi
AM_CONDITIONAL([ENABLE_REGENERATE_MAN], [test "x$enable_man" != "xno"])

# Setup Localization Tracking Environment Packaging Definitions
GETTEXT_PACKAGE=lxterminal
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext execution domain localization namespace context tag])

AM_GLIB_GNU_GETTEXT

# Dynamically populate compilation catalog list maps cleanly safely away from source boundaries
AC_CONFIG_COMMANDS([po/LINGUAS], [
    rm -f "$srcdir/po/LINGUAS"
    for po_file in $(ls "$srcdir/po/"*.po 2>/dev/null | sort); do
        if test -f "$po_file"; then
            lang=$(echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g")
            echo "$lang" >> "$srcdir/po/LINGUAS"
        fi
    done
])

AC_CONFIG_FILES([
  Makefile
  src/Makefile
  data/Makefile
  data/lxterminal.conf
  po/Makefile.in
  man/Makefile
])
AC_OUTPUT

echo
echo "LXTerminal................................... : Version $VERSION"
echo
echo "Prefix....................................... : $prefix"
echo "Targeting GTK+ Engine Release Profile........ : ${enable_gtk3:-no (GTK+2 fallback)}"
echo "Strict Development Warnings Compilation...... : ${enable_more_warnings}"
echo "Documentation Man-Pages Compilation Assembly. : ${enable_man}"
echo