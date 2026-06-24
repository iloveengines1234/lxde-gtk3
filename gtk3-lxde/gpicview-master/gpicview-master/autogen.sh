#!/bin/sh

# Exit immediately if a command exits with a non-zero status
set -e

# Ensure local m4 directory exists
mkdir -p m4

# Print commands as they are executed for easier debugging
set -x

# Run intltoolize to prepare internationalization files
intltoolize --automake --copy --force

# Use autoreconf to run aclocal, autoconf, autoheader, automake, and libtoolize
# -f: force reconstruction
# -i: install missing auxiliary files
autoreconf -fi

# Clean up build cache
rm -rf autom4te.cache