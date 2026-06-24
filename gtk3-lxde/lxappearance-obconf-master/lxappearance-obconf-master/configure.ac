dnl ============================================================================
dnl configure.ac - GNU Autotools Configuration Engine Script
dnl Component: lxappearance-obconf
dnl ============================================================================

AC_PREREQ([2.69])
AC_INIT([lxappearance-obconf], [0.2.4], [http://lxde.org])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Fixed typo: subobject-objects -> subdir-objects
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign no-dist-gzip dist-xz subdir-objects])

AC_PREFIX_DEFAULT([/usr/local])

# Support silent build rules.
AM_SILENT_RULES([yes])

AM_MAINTAINER_MODE([enable])

# Pick compiler specific flags
AM_PROG_CC_C_O
AC_PROG_CC

AC_PROG_INSTALL
LT_PREREQ([2.4])
LT_INIT([disable-static])

# Gettext and Localization Setup
IT_PROG_INTLTOOL([0.40.0])
GETTEXT_PACKAGE=lxappearance-obconf
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package identifier])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19])

# Command-line configuration parameters parsing
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [build lxappearance-obconf with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)

if test "x$enable_debug" = "xyes"; then
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
    if test "x$GCC" = "xyes"; then
        CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
    fi
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700"
else
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

AC_ARG_ENABLE([more_warnings],
    [AS_HELP_STRING([--enable-more-warnings],
        [Add more compiler validation checks @<:@default=no@:>@])],
    [enable_more_warnings="${enableval}"],
    [enable_more_warnings=no]
)

# Core Library/Dependency Validation Checks
PKG_CHECK_MODULES([OPENBOX], [obrender-3.5 >= 3.5 obt-3.5 >= 3.5])
AC_SUBST([OPENBOX_CFLAGS])
AC_SUBST([OPENBOX_LIBS])

# Force-pinning the plugin directly to GTK 3.24.52+ environment parameters
CFLAGS="$CFLAGS -DENABLE_GTK3 -Wno-deprecated-declarations"
gtk_modules="gtk+-3.0 >= 3.24.52"

PKG_CHECK_MODULES([GTK], [$gtk_modules])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

gdk_modules="gdk-3.0 gdk-pixbuf-2.0"
PKG_CHECK_MODULES([GDK_PIXBUF], [$gdk_modules])
AC_SUBST([GDK_PIXBUF_CFLAGS])
AC_SUBST([GDK_PIXBUF_LIBS])

PKG_CHECK_MODULES([GMODULE], [gmodule-export-2.0])
AC_SUBST([GMODULE_CFLAGS])
AC_SUBST([GMODULE_LIBS])

PKG_CHECK_MODULES([LXAPPEARANCE], [lxappearance])
AC_SUBST([LXAPPEARANCE_CFLAGS])
AC_SUBST([LXAPPEARANCE_LIBS])

# Identify lxappearance layout directories from configuration profiles
PKG_CHECK_EXISTS([lxappearance], [
    LXAPPEARANCE_PREFIX=`$PKG_CONFIG --variable=prefix lxappearance`
    LXAPPEARANCE_LIBDIR=`$PKG_CONFIG --variable=libdir lxappearance`/lxappearance
    LXAPPEARANCE_DATADIR=`$PKG_CONFIG --variable=datadir lxappearance`/lxappearance
], [
    LXAPPEARANCE_PREFIX="$prefix"
    LXAPPEARANCE_LIBDIR="$libdir/lxappearance"
    LXAPPEARANCE_DATADIR="$datadir/lxappearance"
])

AC_SUBST([LXAPPEARANCE_LIBDIR])
AC_SUBST([LXAPPEARANCE_DATADIR])

# Safely escape dynamic loader regex configuration tags
LXAPPEARANCE_MODULE="-avoid-version -no-undefined -export-symbols-regex '^[[^_]].*'"
AC_SUBST([LXAPPEARANCE_MODULE])

# Security hardening and strict warning optimization flags
if test "x$enable_more_warnings" = "xyes"; then
    ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=implicit-int -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0"
    if test "x$enable_debug" != "xyes"; then
        ADDITIONAL_FLAGS="$ADDITIONAL_FLAGS -Wp,-D_FORTIFY_SOURCE=2"
    fi
fi
AC_SUBST([ADDITIONAL_FLAGS])

AC_CONFIG_FILES([
    Makefile
    po/Makefile.in
])
AC_OUTPUT

AC_MSG_RESULT([
========================================================================
Configuration Complete (lxappearance-obconf):
    Source Code Path:    ${srcdir}
    Installation Prefix: ${prefix}
    Compiler Variant:    ${CC}
    Target Toolkit:      GTK+ 3.24.52+ (Pinned)
    Debugging Modes:     ${enable_debug}
========================================================================
Type "make" to compile the plugin module layout structures.
])