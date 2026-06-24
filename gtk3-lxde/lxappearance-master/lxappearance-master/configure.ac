#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([lxappearance],[0.6.4],[http://lxde.org/])
AM_INIT_AUTOMAKE([1.11 -Wall -Werror foreign subdir-objects no-dist-gzip dist-xz])

# intltool configuration
IT_PROG_INTLTOOL([0.40.0])

# Support silent build rules.
AM_SILENT_RULES([yes])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([.])

# C compiler setup
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Setup basic system specifications
AC_SYS_LARGEFILE

AC_ARG_ENABLE([man],
    [AS_HELP_STRING([--enable-man],[regenerate roff man pages from Docbook @<:@default=no@:>@])],
    [case "${enableval}" in
      yes) enable_man=yes ;;
      no)  enable_man=no ;;
      *)   AC_MSG_ERROR([bad value "${enableval}" for --enable-man, use "no" (default) or "yes".]) ;;
    esac],
    [enable_man=no])

AC_ARG_ENABLE([more_warnings],
    [AS_HELP_STRING([--enable-more-warnings],[Add more strict compiler warnings @<:@default=no@:>@])],
    [enable_more_warnings="${enableval}"],
    [enable_more_warnings=no])

# Base core packages checking loops
PKG_CHECK_MODULES([XLIB], [x11])
AC_SUBST([XLIB_CFLAGS])
AC_SUBST([XLIB_LIBS])

# Force-pinning the target directly to GTK 3.24+ environment architectures
CFLAGS="$CFLAGS -DENABLE_GTK3"
gtk_modules="gtk+-3.0 >= 3.24.52 gthread-2.0"

PKG_CHECK_MODULES([GTK], [$gtk_modules])
AC_SUBST([GTK_CFLAGS])
AC_SUBST([GTK_LIBS])

AC_ARG_ENABLE([dbus],
    [AS_HELP_STRING([--enable-dbus],[Use D-Bus support @<:@default=yes@:>@])],
    [enable_dbus="${enableval}"],
    [enable_dbus=yes])

if test x"$enable_dbus" = x"yes"; then
    PKG_CHECK_MODULES([DBUS], [dbus-1 >= 0.95])
    CFLAGS="$CFLAGS -DENABLE_DBUS"
fi
AC_SUBST([DBUS_CFLAGS])
AC_SUBST([DBUS_LIBS])

gmodule_modules="gmodule-export-2.0"
PKG_CHECK_MODULES([GMODULE], [$gmodule_modules])
AC_SUBST([GMODULE_CFLAGS])
AC_SUBST([GMODULE_LIBS])
AC_SUBST([gtk_modules])

if test x"$enable_more_warnings" = x"yes"; then
    ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=implicit-int -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2"
fi
AC_SUBST([ADDITIONAL_FLAGS])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],[build lxappearance with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no])

if test "$enable_debug" = "yes"; then
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700"
else
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

# Dynamically resolve translations tracking manifests safely
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`; do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

GETTEXT_PACKAGE=lxappearance
AC_SUBST([GETTEXT_PACKAGE])
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Gettext package.])

if test x"$enable_man" = x"yes"; then
    AC_PATH_PROG([XSLTPROC], [xsltproc])
    if test -z "$XSLTPROC"; then
        AC_MSG_ERROR([xsltproc is required to regenerate the pre-built man page; consider --disable-man])
    fi

    JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
        [DocBook XML DTD V4.1.2], [], [AC_MSG_ERROR([DocBook XML DTD is required to regenerate the pre-built man page; consider --disable-man])])
    JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
        [DocBook XSL Stylesheets >= 1.70.1], [], [AC_MSG_ERROR([DocBook XSL Stylesheets are required to regenerate the pre-built man page; consider --disable-man])])

    rm -f $srcdir/man/lxappearance.1
fi

AM_CONDITIONAL([ENABLE_MAN], [test "x$enable_man" != "xno"])

AC_CONFIG_FILES([
    Makefile
    data/Makefile
    data/ui/Makefile
    data/ui/about.glade
    data/lxappearance.pc
    man/Makefile
    po/Makefile.in
    src/Makefile
])

AC_OUTPUT

echo
echo lxappearance....................... : Version $VERSION
echo
echo Enable debug....................: "$enable_debug"
echo Enable D-Bus support............: "$enable_dbus"
echo Prefix..........................: $prefix
echo
echo The binary will be installed in $prefix/bin
echo
echo http://lxde.org/
echo
echo "Ready to make lxappearance. Type 'make' to continue."
echo