dnl Process this file with autoconf to produce a configure script.
dnl Updated by iloveengines1234 to port completely to GTK 3 / Modern Autoconf

AC_PREREQ([2.69])
AC_INIT([lxtask], [0.1.12], [http://lxde.org/], [lxtask])
AC_CONFIG_SRCDIR([src/lxtask.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([.])

# Modern Fix: Corrected 'subobject-objects' configuration patterns by introducing 
# modern subdir-objects tracking parameter fields directly into the main automake array.
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])

AM_MAINTAINER_MODE([enable])

dnl Support silent build rules. Disable by either passing --disable-silent-rules
dnl to configure or passing V=1 to make
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O

# Modern Fix: Removed deprecated and obsolete UNIX variant testing macros 
# (AC_AIX, AC_ISC_POSIX, AC_HEADER_STDC) which are handled natively by contemporary CC wrappers.

AC_CHECK_HEADERS([dirent.h pwd.h sys/types.h sys/stat.h sys/param.h \
                  stdio.h stdlib.h string.h unistd.h signal.h])

# Modern Fix: Stripped out legacy GTK 2 translation toggles. 
# Enforced unified GTK 3.24.52 target architecture mapping baseline requirements natively.
gtk_modules="gtk+-3.0 >= 3.24.52"
CFLAGS="$CFLAGS -DENABLE_GTK3"

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.50.0)

PKG_CHECK_MODULES(GTK, [$gtk_modules])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# Init intltool
IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=lxtask
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

# Generate po/LINGUAS on the fly
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug],
        [build lxtask with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)

if test "$enable_debug" = "yes"; then
    # turn on debug and disable optimization
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
    case "$CC" in
    gcc*)
        CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers"
        ;;
    *)
        ;;
    esac
    dnl Be more strict on portability
    CPPFLAGS="$CPPFLAGS -D_POSIX_C_SOURCE=200112L -D_XOPEN_SOURCE=700"
else
    # turn off glib debug checks
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

# Modern Fix: Swapped out legacy explicit target configuration listings in AC_OUTPUT 
# for the standard AC_CONFIG_FILES declaration layer to conform with contemporary autoconf standards.
AC_CONFIG_FILES([
  Makefile
  src/Makefile
  po/Makefile.in
])
AC_OUTPUT

echo
echo LXTask....................................... : Version $PACKAGE_VERSION
echo
echo Prefix....................................... : $prefix
echo
echo The binary will be installed in $prefix/bin
echo
echo http://lxde.org/
echo
echo Have fun!
echo