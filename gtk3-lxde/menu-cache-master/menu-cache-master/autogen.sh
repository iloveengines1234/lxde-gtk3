#!/bin/sh
# Updated autogen orchestration script requiring modern automake versions (>= 1.15)

test -n "$SRC_DIR" || SRC_DIR=$(dirname "$0")
test -n "$SRC_DIR" || SRC_DIR=.

OLD_DIR=$(pwd)
cd "$SRC_DIR"

AUTOMAKE=${AUTOMAKE:-automake}
AM_INSTALLED_VERSION=$($AUTOMAKE --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

# Version Fix: Enforce a strict requirement for version 1.15 up to 1.99+
case "$AM_INSTALLED_VERSION" in
    1.1[5-9]|1.[2-9][0-9])
    ;;
    *)
    echo
    echo "You must have automake >= 1.15 installed."
    echo "Install the appropriate package for your distribution,"
    echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
    exit 1
    ;;
esac

if [ "x${ACLOCAL_DIR}" != "x" ]; then
    export ACLOCAL_AMFLAGS="-I ${ACLOCAL_DIR}"
fi

# Ensure local macro space exists safely
test -d m4 || mkdir m4

# Infrastructure hooks validation for GTK documentation generation
if gtkdocize --copy; then
    echo "Files needed by gtk-doc are generated."
else
    echo "You need gtk-doc to build this package."
    echo "https://developer.gnome.org/gtk-doc-manual/"
    exit 1
fi

set -x

# Execute libtoolize safely before configuration generation pass
libtoolize -c --automake --force

# Use autoreconf to automatically orchestrate aclocal, autoheader, automake, and autoconf
autoreconf --install --force --verbose

# Completely clear transient configuration cache spaces
rm -rf autom4te.cache

if test -n "$DOCONFIGURE"; then
    ./configure "$@"
fi

cd "$OLD_DIR"