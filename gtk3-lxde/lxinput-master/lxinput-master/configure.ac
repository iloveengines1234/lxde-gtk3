dnl Process this file with autoconf to produce a configure script.
dnl Updated by iloveengines1234 to port completely to GTK 3 / Modern Autoconf

AC_PREREQ([2.69])
AC_INIT([lxinput], [0.3.6], [http://lxde.org/])
AC_CONFIG_SRCDIR([src/lxinput.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])

# Support silent build rules. Disable by either passing --disable-silent-rules
# to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AM_MAINTAINER_MODE([enable])

# Checks for programs.
AC_PROG_CC

# Internationalization support via intltool
IT_PROG_INTLTOOL([0.40.0])

AC_ARG_ENABLE(man,
       [AS_HELP_STRING([--enable-man],
               [regenerate roff man pages from Docbook @<:@default=no@:>@])],
       [enable_man="${enableval}"],
       [enable_man=no]
)

AC_ARG_ENABLE(more_warnings,
       [AS_HELP_STRING([--enable-more-warnings],
               [Add more warnings @<:@default=no@:>@])],
       [enable_more_warnings="${enableval}"],
       [enable_more_warnings=no]
)

# Enforce unified GTK 3.24.52 target architecture mapping
PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= 3.24])
CFLAGS="$CFLAGS -DENABLE_GTK3"

# Checks for core dependencies
PKG_CHECK_MODULES([GTK], [$gtk_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# Explicitly fetch core GLib/GObject modules required by our async handlers
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.50.0 gobject-2.0 >= 2.50.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES([XLIB], [x11])
AC_SUBST(XLIB_CFLAGS)
AC_SUBST(XLIB_LIBS)

if test x"$enable_more_warnings" = x"yes"; then
    ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2"
fi
AC_SUBST(ADDITIONAL_FLAGS)

# Generate po/LINGUAS on the fly
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

GETTEXT_PACKAGE=lxinput
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

if test x"$enable_man" = x"yes"; then
    AC_PATH_PROG([XSLTPROC], [xsltproc])
    if test -z "$XSLTPROC"; then
        AC_MSG_ERROR([xsltproc is required to regenerate the pre-built man page; consider --disable-man])
    fi

    dnl check for DocBook DTD and stylesheets in the local catalog.
    JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
        [DocBook XML DTD V4.1.2], [], AC_MSG_ERROR([DocBook XML DTD is required to regenerate the pre-built man page; consider --disable-man]))
    JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
        [DocBook XSL Stylesheets >= 1.70.1], [], AC_MSG_ERROR([DocBook XSL Stylesheets are required to regenerate the pre-built man page; consider --disable-man]))

    rm -f $srcdir/man/lxinput.1
fi

AM_CONDITIONAL(ENABLE_REGENERATE_MAN, test "x$enable_man" != "xno")

AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug],
        [build lxinput with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)

if test "$enable_debug" = "yes"; then
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
    case "$CC" in
    gcc*)
        CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
        ;;
    *)
        ;;
    esac
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700"
else
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

AC_CONFIG_FILES([
Makefile
src/Makefile
po/Makefile.in
man/Makefile
])
AC_OUTPUT
