#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lxde-common], [0.99.3], [http://lxde.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-dist-gzip dist-xz])
AC_CONFIG_SRCDIR([startlxde.in])

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_LN_S

# Documentation build features toggles
AC_ARG_ENABLE([man],
    [AS_HELP_STRING([--enable-man], [regenerate roff man pages from Docbook @<:@default=no@:>@])],
    [enable_man="${enableval}"],
    [enable_man=no]
)

# Xsessions data path configuration mapping
AC_ARG_WITH([xsession-path],
    [AS_HELP_STRING([--with-xsession-path=PATH], [set the install path of xsession data [[DATAROOTDIR/xsessions]]])],
    [test "x$withval" != x && xsesspath="$withval"],
    [xsesspath="\${datarootdir}/xsessions"]
)
AC_SUBST([xsesspath])

if test x"$enable_man" = x"yes"; then
    AC_PATH_PROG([XSLTPROC], [xsltproc])
    if test -z "$XSLTPROC"; then
        enable_man=no
    fi

    dnl Check for DocBook DTD and stylesheets using our modernized acinclude.m4 macros
    JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
        [DocBook XML DTD V4.1.2], [], [enable_man=no])
    JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
        [DocBook XSL Stylesheets >= 1.70.1], [], [enable_man=no])
fi

AM_CONDITIONAL([ENABLE_REGENERATE_MAN], [test "x$enable_man" != "xno"])

dnl Fix invalid sysconfdir when --prefix=/usr
if test `eval "echo $sysconfdir"` = /usr/etc; then
    sysconfdir=/etc
fi

GETTEXT_PACKAGE=lxde-common
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

# Re-architected PO tracking template matching routine safely
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort 2>/dev/null`; do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    if test -n "$lang"; then
        echo $lang >> $srcdir/po/LINGUAS
    fi
done

# Fixed: Removed the clashing AM_GLIB_GNU_GETTEXT macro block to fix tracking conflicts
IT_PROG_INTLTOOL([0.40.0])

dnl Direct fallback tracking override for static installations
USE_NLS=no
AC_SUBST([USE_NLS])

# Fixed: Modernized output structure mapping via explicit AC_CONFIG_FILES call
AC_CONFIG_FILES([
  Makefile
  startlxde
  lxde-logout
  LXDE.desktop.in
  lxpanel/panel
  openbox/rc.xml
  openbox-lxde
  pcmanfm/pcmanfm.conf
  man/Makefile
  po/Makefile.in
])

AC_OUTPUT

if test x"$sysconfdir" != x'/etc'; then
    echo
    echo 'Warning: sysconfdir is not /etc.'
    echo 'Please consider passing --sysconfdir=/etc to configure.'
    echo 'Otherwise default config files will be installed to the wrong place.'
    echo
fi