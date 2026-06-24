dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lxlauncher], [0.2.8], [http://lxde.org])
AC_CONFIG_SRCDIR([src/lxlauncher.c])
AC_CONFIG_HEADERS([config.h])

# Initialize the Automake layout environment cleanly
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects])

# Support silent build rules out of the box (enable verbose logging with V=1)
AM_SILENT_RULES([yes])

AM_MAINTAINER_MODE([enable])

# Init standard dynamic package translation framework
IT_PROG_INTLTOOL([0.40.0])

# Checks for core development compilation programs
AC_PROG_CC

# Setup configurable compilation warnings parameter toggle
AC_ARG_ENABLE(more_warnings,
       [AS_HELP_STRING([--enable-more-warnings],
               [Add more compiler security tracking warnings @<:@default=no@:>@])],
       [enable_more_warnings="${enableval}"],
       [enable_more_warnings=no]
)

# --- Consolidated Core GTK3 Toolkit Validation Engine ---
# Synchronized cleanly to export $(GTK3_CFLAGS) and $(GTK3_LIBS) matching makefile setups
PKG_CHECK_MODULES([GTK3], [gtk+-3.0 >= 3.24.52])

# --- Localized LXDE Application Menu Cache Dependency ---
# Name matched exactly to synchronize with $(MENU_CFLAGS) and $(MENU_LIBS) definitions
PKG_CHECK_MODULES([MENU], [libmenu-cache])

# Verify specific menu cache subsystem functionality limits
LIBS_save="${LIBS}"
LIBS="${LIBS} ${MENU_LIBS}"
AC_CHECK_FUNCS([menu_cache_dir_list_children])
LIBS="${LIBS_save}"

# --- Strict Compiler Security Hardening Profiler Flags ---
if test x"$enable_more_warnings" = x"yes"; then
    ADDITIONAL_FLAGS="-Wall -Werror=all -Werror=format -Werror=implicit-function-declaration -Werror=missing-braces -Werror=parentheses -Werror=return-type -Werror=strict-aliasing -Werror=switch -Wuninitialized -Werror=unused-label -Werror=unused-value -Wextra -Wno-missing-field-initializers -Wno-unused-parameter -Werror=missing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Werror=inline -Werror=format-nonliteral -Wformat-nonliteral -Werror=format-security -Wformat-security -Winit-self -Werror=missing-include-dirs -Werror=undef -Werror=aggregate-return -Wmissing-format-attribute -Werror=nested-externs -fno-strict-aliasing -fmessage-length=0 -Wp,-D_FORTIFY_SOURCE=2"
fi
AC_SUBST(ADDITIONAL_FLAGS)

# --- Internationalization Setup ---
GETTEXT_PACKAGE=lxlauncher
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package identifier context.])

# Dynamically generate local localization compilation maps on the fly
rm -f $srcdir/po/LINGUAS
for po_file in `ls $srcdir/po/*.po | sort`;
do
    lang=`echo "$po_file" | sed "s|.*/po/\(.*\)\.po|\1|g"`
    echo $lang >> $srcdir/po/LINGUAS
done

# --- Target Build Workspace Commit Sequence ---
AC_CONFIG_FILES([
Makefile
settings.conf
src/Makefile
po/Makefile.in
data/lxlauncher.1
])
AC_OUTPUT