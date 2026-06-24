# -*- Autoconf -*-
# configure.ac for lxhotkey
# Copyright (C) 2016-2021   Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
# Copyright (C) 2026        LXDE Maintainers Group

AC_PREREQ([2.69])
AC_INIT([lxhotkey], [0.1.2], [http://lxde.org/])
AC_CONFIG_SRCDIR([src/lxhotkey.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Initialize the Automake layout environment cleanly
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects no-dist-gzip dist-xz])

# Support silent build rules out of the box (enable verbose logging with V=1)
AM_SILENT_RULES([yes])

# Force to dynamic shared library plugin building
AC_DISABLE_STATIC

# Checks for core development compilation programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_CC_C_O
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# Initialize libtool layer contexts safely for runtime modular expansions
LT_PREREQ([2.2])
LT_INIT

# --- Internationalization Support Setup ---
GETTEXT_PACKAGE=lxhotkey
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package identifier.])

# Init standard dynamic package translation framework natively
IT_PROG_INTLTOOL([0.40.0], [no-xml])

# Hardened the dynamic translation list loop to use ultra-portable,
# native shell pattern matching variable extensions instead of external subshells.
AS_IF([test -d "$srcdir/po"], [
    rm -f "$srcdir/po/LINGUAS"
    for po_file in "$srcdir"/po/*.po; do
        if test -f "$po_file"; then
            lang_base="${po_file##*/}"
            lang="${lang_base%.po}"
            echo "$lang" >> "$srcdir/po/LINGUAS"
        fi
    done
])

# Handle --enable-debug option loops cleanly
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [enable debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)

if test "$enable_debug" = "yes"; then
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG"
    CFLAGS="$CFLAGS -O0 -g"
    case "$CC" in
        gcc*|clang*)
            CFLAGS="$CFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Werror=implicit-function-declaration"
            ;;
        *)
            ;;
    esac
else
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

# --- Core Platform Library Validations ---
pkg_modules="libfm >= 1.2.0"
PKG_CHECK_MODULES([PACKAGE], [$pkg_modules])

# --- GTK Toolkit Version Integration Selection ---
AC_ARG_WITH([gtk],
    [AS_HELP_STRING([--with-gtk=VER], [build 'gtk' GUI, choose GTK version for build (2 or 3) @<:@default=3@:>@])],
    [ac_with_gtk="${withval}"],
    [ac_with_gtk=3]
)

with_gtk=no
case "$ac_with_gtk" in
    3|3.*)
        gtk_modules="gtk+-3.0 >= 3.24.52"
        ;;
    2|2.*)
        # CRITICAL REFINEMENT: Enforce strict point-release baseline for the legacy stack branch
        gtk_modules="gtk+-2.0 >= 2.24.33"
        ;;
    *)
        AC_MSG_ERROR([Unsupported GTK variant target specified: $ac_with_gtk])
        ;;
esac

PKG_CHECK_MODULES([GTK], [$gtk_modules], [with_gtk=yes], [with_gtk=no])

if test "$with_gtk" = "yes" && test "$ac_with_gtk" = "3"; then
    CFLAGS="$CFLAGS -Wno-deprecated-declarations"
fi

AM_CONDITIONAL([WITH_GTK], [test x$with_gtk = xyes])

# Verify standard text manipulation libraries are available
AC_CHECK_LIB([unistring], [ulc_fprintf])

# Suppress redundant direct link overhead paths via link optimizations
AS_CASE([$LDFLAGS], [*-Wl,--as-needed*], [], [
    LDFLAGS="$LDFLAGS -Wl,--as-needed"
])

# Fix invalid sysconfdir fallback bounds if configured with prefix=/usr
if test "$(eval echo $sysconfdir)" = "/usr/etc"; then
    sysconfdir=/etc
fi

# Emit build system output assets cleanly
AC_CONFIG_FILES([
    Makefile
    lxhotkey.pc
    src/Makefile
    plugins/Makefile
    po/Makefile.in
    man/lxhotkey.1
])
AC_OUTPUT