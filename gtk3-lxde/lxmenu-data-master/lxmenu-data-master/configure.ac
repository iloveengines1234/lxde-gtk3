# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([lxmenu-data], [0.1.7], [https://github.com/lxde/lxmenu-data])

# FIXED: Synchronized macro storage target directory for tracked custom dependencies
AC_CONFIG_MACRO_DIR([m4])

# Initialize the Automake runtime profile engine layout cleanly
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz subdir-objects silent-rules])

# Support silent build rules out of the box (enable verbose logging with V=1)
AM_SILENT_RULES([yes])

# --- Translation Framework Configuration ---
GETTEXT_PACKAGE=lxmenu-data
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

# Verify system internationalization framework availability
IT_PROG_INTLTOOL([0.40.0])

# FIXED: Hardened the dynamic translation list loop to use ultra-portable, 
# native shell pattern matching variable extensions instead of external subshells.
AS_IF([test -d "$srcdir/po"], [
    rm -f "$srcdir/po/LINGUAS"
    for po_file in "$srcdir"/po/*.po; do
        if test -f "$po_file"; then
            # Remove directory path prefix
            lang_base="${po_file##*/}"
            # Strip trailing extension suffix cleanly without launching an external subshell
            lang="${lang_base%.po}"
            echo "$lang" >> "$srcdir/po/LINGUAS"
        fi
    done
])

# --- Target Build Workspace Commit Sequence ---
AC_CONFIG_FILES([
Makefile
layout/Makefile
desktop-directories/Makefile
po/Makefile.in
])
AC_OUTPUT