#!/bin/sh
# Updated for modern autotools and GTK 3.24+ compilation environments

AUTOMAKE=${AUTOMAKE:-automake}
AM_INSTALLED_VERSION=$($AUTOMAKE --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

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
    export ACLOCAL_AMFLAGS="-I ${ACLOCAL_DIR}"
fi

set -x

# Prepare internationalization tracking files
intltoolize -c --automake --force

# Use autoreconf to orchestrate aclocal, autoheader, automake, and autoconf natively
autoreconf --install --force --verbose

# Clean workspace transient cache tables completely
rm -rf autom4te.cache