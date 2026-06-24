#!/bin/sh

set -e

# Change to the directory containing the script
cd "$(dirname "$0")"

# Ensure clean build environment tracking
rm -rf autom4te.cache

# Run intltoolize explicitly if the project still relies on intltool
if command -v intltoolize >/dev/null 2>&1; then
    echo "Running intltoolize..."
    intltoolize --force --copy --automake
fi

# Run autoreconf to handle aclocal, autoconf, automake, and autoheader smoothly
echo "Running autoreconf..."
autoreconf --force --install --verbose

echo "
Ready to build! Now you can run:
  ./configure
  make
"