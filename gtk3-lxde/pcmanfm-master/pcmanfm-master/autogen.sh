#!/bin/sh

# Exit immediately if any command fails
set -e

srcdir=$(dirname "$0")
test -z "$srcdir" && srcdir=.

olddir=$(pwd)
cd "$srcdir"

# Ensure essential build tools are present
printf "Checking for autotools... "
for tool in autoreconf automake autoconf; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: '$tool' not found. Please install autotools."
        exit 1
    fi
done
echo "done."

# intltoolize is legacy but retained here if your po/ directory still requires it.
# Modern GTK 3 apps usually migrate to standard gettext (AM_GNU_GETTEXT).
if command -v intltoolize >/dev/null 2>&1; then
    echo "Running intltoolize..."
    intltoolize --force --copy --automake
fi

# Run autoreconf to handle aclocal, autoheader, automake, and autoconf automatically
echo "Running autoreconf..."
autoreconf --force --install --verbose

cd "$olddir"

# Automatically run configure if requested
if [ "$1" = "--configure" ]; then
    echo "Running configure..."
    "$srcdir/configure" "$@"
else
    echo "Now type './configure' to configure the package."
fi