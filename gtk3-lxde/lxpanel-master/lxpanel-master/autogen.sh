#!/bin/sh

AUTOMAKE=${AUTOMAKE:-automake}
AM_INSTALLED_VERSION=$($AUTOMAKE --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

# Robust numeric evaluation supporting any version >= 1.11
# Split version into major and minor elements
AM_MAJOR=$(echo "$AM_INSTALLED_VERSION" | cut -d. -f1)
AM_MINOR=$(echo "$AM_INSTALLED_VERSION" | cut -d. -f2)

# Verify minimum threshold rules (Major must be > 1, or if 1, Minor must be >= 11)
if [ "$AM_MAJOR" -lt 1 ] || { [ "$AM_MAJOR" -eq 1 ] && [ "$AM_MINOR" -lt 11 ]; }; then
    echo
    echo "You must have automake >= 1.11 installed (Detected: $AM_INSTALLED_VERSION)."
    echo "Install the appropriate package for your distribution,"
    echo "or get the source tarball at http://ftp.gnu.org/gnu/automake/"
    exit 1
fi

if [ "x${ACLOCAL_DIR}" != "x" ]; then
    ACLOCAL_ARG="-I ${ACLOCAL_DIR}"
fi

test -d m4 || mkdir m4
set -x

# Using modern autoreconf tool chaining fallback syntax if specific versions aren't overridden
${ACLOCAL:-aclocal} ${ACLOCAL_ARG}
${AUTOHEADER:-autoheader} --force

# Modern configurations favor 'libtoolize' natively on Linux and fallback to 'glibtoolize' on macOS
if command -v libtoolize >/dev/null 2>&1; then
    AUTOMAKE=$AUTOMAKE libtoolize -c --automake --force
else
    AUTOMAKE=$AUTOMAKE glibtoolize -c --automake --force
fi

intltoolize -c --automake --force
$AUTOMAKE --add-missing --copy
${AUTOCONF:-autoconf}

rm -rf autom4te.cache