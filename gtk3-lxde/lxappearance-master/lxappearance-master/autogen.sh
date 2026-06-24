#!/bin/sh
# Modernized autogen.sh for LXAppearance Port

# Check for autoreconf, which automatically coordinates aclocal, autoheader, automake, and autoconf
if ! which autoreconf > /dev/null 2>&1; then
    echo "Error: You must have autoconf and automake installed."
    exit 1
fi

# Ensure upstream gettext infrastructure is available instead of deprecated intltool
if ! which autopoint > /dev/null 2>&1; then
    echo "Error: You must have gettext (autopoint) installed."
    exit 1
fi

set -x

# Clean out old cache directories
rm -rf autom4te.cache

# Bring in modern gettext m4 files and build rules
autopoint --force

# Run all Autotools utilities in the correct sequence with installation flags
autoreconf --force --install --verbose

set +x
echo "Ready to run ./configure"