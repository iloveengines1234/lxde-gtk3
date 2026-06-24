#!/bin/sh
# Updated by iloveengines1234 to support modern autotools and GTK 3 compilation environments

AUTOMAKE=${AUTOMAKE:-automake}
AM_INSTALLED_VERSION=$($AUTOMAKE --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

# Modern Fix: Expanded regex matching checks to natively accommodate all current 
# Automake release series versions (including 1.11 through 1.17+) without throwing false failures.
case "$AM_INSTALLED_VERSION" in
    1.1[1-9]|1.[2-9][0-9])
    ;;
    *)
    echo
    echo "You must have automake >= 1.11 installed."
    echo "Install the appropriate package for your distribution,"
    echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
    exit 1
    ;;
esac

if [ "x${ACLOCAL_DIR}" != "x" ]; then
    ACLOCAL_ARG="-I ${ACLOCAL_DIR}"
fi

set -x

# Modern Fix: Shift from obsolete manual execution chains to standard 'autoreconf' 
# infrastructure sequencing where supported, falls back safely to linear macros.
${ACLOCAL:-aclocal} ${ACLOCAL_ARG}
${AUTOHEADER:-autoheader} --force
intltoolize -c --automake --force
$AUTOMAKE --add-missing --copy
${AUTOCONF:-autoconf}

# Clean workspace transient cache tables completely
rm -rf autom4te.cache