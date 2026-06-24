dnl Process this file with autoconf to produce a configure script.
dnl Updated by iloveengines1234 to port completely to GTK 3.24.52 / Modern Autoconf

AC_PREREQ([2.69])
AC_INIT([lxsession], [0.5.6], [https://github.com/lxde/lxsession/issues], [lxsession])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# Modern Fix: Corrected 'subobject-objects' typo to the valid 'subdir-objects' option.
# Enforces clean encapsulation across parallel workspace builders.
AM_INIT_AUTOMAKE([1.11.1 -Wall -Werror foreign no-dist-gzip dist-xz subdir-objects])

# Support silent build rules. Disable by passing --disable-silent-rules
AM_SILENT_RULES([yes])

# Infrastructure toolchain and compiler initialization
AC_PROG_CC
AC_PROG_INSTALL
AM_PROG_CC_C_O
AM_PROG_AR

# Modern Fix: Initialize Libtool infrastructure early to ensure dynamic layout 
# link targets resolve safely without breaking architecture boundaries.
LT_INIT([disable-static])

# Localization engine bootstrapping
IT_PROG_INTLTOOL([0.40.0],[no-xml])

AC_CHECK_FUNC([mkstemp],
   AC_DEFINE(HAS_MKSTEMP,1,[Define to 1 if you have the 'mkstemp' function.]))

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

if test x"$enable_more_warnings" = x"yes"; then
    ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2"
fi
AC_SUBST(ADDITIONAL_FLAGS)

# Core Platform Dependency Validations
PKG_CHECK_MODULES(X11, [x11])
AC_SUBST(X11_CFLAGS)
AC_SUBST(X11_LIBS)

PKG_CHECK_MODULES(GLIB, [glib-2.0 >= 2.50.0 gobject-2.0 >= 2.50.0])
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(GIO, [gio-unix-2.0 >= 2.50.0])
AC_SUBST(GIO_CFLAGS)
AC_SUBST(GIO_LIBS)

# Modern Fix: Removed deprecated and insecure dbus-glib-1 verification requirements.
# The codebase relies completely on native, robust internal GDBus structures.

AC_ARG_ENABLE(
    [gtk],
    AS_HELP_STRING([--enable-gtk],
                   [Build with gtk support (default: yes)]),
    use_gtk=$enableval, use_gtk="yes")

AM_CONDITIONAL(USE_GTK, test "$use_gtk" = "yes")

# Modern Fix: Allocate an isolated internal variable wrapper to store compilation 
# architecture symbols safely without clobbering user-defined flags.
AM_CFLAGS=""

if test "x$use_gtk" = "xno" ; then
    enable_gtk3="no"
    use_buildin_clipboard="no"
    use_buildin_polkit="no"
else
    # Automatically enforce unified GTK 3.24.52 target architecture mapping
    enable_gtk3="yes"
    AM_CFLAGS="$AM_CFLAGS -DUSE_GTK3"
    gtk_modules="gtk+-3.0 >= 3.24.52"
    VALA_GTK_LIBS="gtk+-3.0"

    PKG_CHECK_MODULES(GTK, [$gtk_modules])
    AC_SUBST(GTK_CFLAGS)
    AC_SUBST(GTK_LIBS)
    AC_SUBST(VALA_GTK_LIBS)

    polkit_modules="polkit-agent-1"
    PKG_CHECK_MODULES(POLKIT, [$polkit_modules])
    AC_SUBST(POLKIT_CFLAGS)
    AC_SUBST(POLKIT_LIBS)

    AC_ARG_ENABLE(
        [buildin-clipboard],
        AS_HELP_STRING([--enable-buildin-clipboard],
                       [Build with build-in clipboard support (default: no)]),
        use_buildin_clipboard=$enableval, use_buildin_clipboard="no")

    AC_ARG_ENABLE(
        [buildin-polkit],
        AS_HELP_STRING([--enable-buildin-polkit],
                       [Build with build-in polkit-agent support (default: no)]),
        use_buildin_polkit=$enableval, use_buildin_polkit="no")

    AC_ARG_ENABLE(
        [advanced-notifications],
        AS_HELP_STRING([--enable-advanced-notifications],
                       [Build with advanced notification using indicators and libnotify (default: no)]),
        use_advanced_notifications=$enableval, use_advanced_notifications="no")
fi

AC_SUBST(AM_CFLAGS)

AM_CONDITIONAL(USE_BUILDIN_CLIPBOARD, test "$use_buildin_clipboard" = "yes")
AM_CONDITIONAL(USE_BUILDIN_POLKIT, test "$use_buildin_polkit" = "yes")
AM_CONDITIONAL(USE_GTK3, test "$enable_gtk3" = "yes")
AM_CONDITIONAL(USE_ADVANCED_NOTIFICATIONS, test "$use_advanced_notifications" = "yes")

if test "$use_advanced_notifications" = "yes"; then
    PKG_CHECK_MODULES(INDICATORS, [indicator-0.4 >= 0.4.93 appindicator-0.1])
    AC_SUBST(INDICATORS_CFLAGS)
    AC_SUBST(INDICATORS_LIBS)

    PKG_CHECK_MODULES(LIBNOTIFY, [libnotify])
    AC_SUBST(LIBNOTIFY_CFLAGS)
    AC_SUBST(LIBNOTIFY_LIBS)
fi

# Standardize Autostart visibility toggles as clean target string parameters
if test "$use_buildin_polkit" = "yes"; then
    HIDE_LXPOLKIT_AUTOSTART="true"
else
    HIDE_LXPOLKIT_AUTOSTART="false"
fi
AC_SUBST(HIDE_LXPOLKIT_AUTOSTART)

# Modernized target minimum to prevent code generation traps in newer Vala formats
AM_PROG_VALAC([0.40.0])
if test "x$VALAC" = "x"; then
    AC_MSG_ERROR([You need Vala compiler >= 0.40.0])
else
    VALAFLAGS="$VALAFLAGS --enable-deprecated"
    AC_SUBST(VALAFLAGS)
fi

# Debug tracking configurations
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug],[build lxsession with debug support @<:@default=no@:>@])],
    [enable_debug="${enableval}"],
    [enable_debug=no]
)
if test "$enable_debug" = "yes"; then
    CPPFLAGS="$CPPFLAGS -DG_ENABLE_DEBUG -O0 -g"
else
    CPPFLAGS="$CPPFLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS -DG_DISABLE_CAST_CHECKS"
fi

GETTEXT_PACKAGE=lxsession
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

if test x"$enable_man" = x"yes"; then
    AC_PATH_PROG([XSLTPROC], [xsltproc])
    if test -z "$XSLTPROC"; then
        AC_MSG_ERROR([xsltproc is required to regenerate the pre-built man pages; consider --disable-man])
    fi

    dnl Check for DocBook DTD and stylesheets in the local catalog map space.
    JH_CHECK_XML_CATALOG([-//OASIS//DTD DocBook XML V4.1.2//EN],
        [DocBook XML DTD V4.1.2], [], AC_MSG_ERROR([DocBook XML DTD is required to regenerate the pre-built man pages; consider --disable-man]))
    JH_CHECK_XML_CATALOG([http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl],
        [DocBook XSL Stylesheets >= 1.70.1], [], AC_MSG_ERROR([DocBook XSL Stylesheets are required to regenerate the pre-built man pages; consider --disable-man]))

    rm -f $srcdir/man/lxsession.1
    rm -f $srcdir/man/lxsession-logout.1
fi

AM_CONDITIONAL(ENABLE_REGENERATE_MAN, test "x$enable_man" != "xno")

dnl Fix invalid sysconfdir when --prefix=/usr
if test `eval "echo $sysconfdir"` = /usr/etc
then
    sysconfdir=/etc
fi

# Conforms with contemporary autoconf configuration layout generation standards
AC_CONFIG_FILES([
  Makefile
  data/Makefile
  data/lxpolkit.desktop.in
  man/Makefile
  po/Makefile.in
])
AC_OUTPUT

echo
echo LXSession.................................... : Version $PACKAGE_VERSION
echo
echo Prefix....................................... : $prefix
echo Sysconfdir................................... : $sysconfdir
echo GTK support.................................. : $use_gtk
echo GTK Clipboard build-in support................. : $use_buildin_clipboard
echo GTK Polkit-agent build-in support............. : $use_buildin_polkit
echo Enable GTK3 support.......................... : $enable_gtk3
echo Enable debug................................. : $enable_debug
echo Man pages generation support................. : $enable_man
echo
echo The binary will be installed in $prefix/bin
echo
echo https://lxde.org/
echo
echo Have fun!
echo