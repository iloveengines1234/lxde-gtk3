#!/bin/sh
# Modernized autogen.sh for lxmenu-data

set -e

# Determine source directory context safely
SRC_DIR=$(dirname "$0")
test -n "$SRC_DIR" || SRC_DIR=.

OLD_DIR=$(pwd)
cd "$SRC_DIR"

# --- Hardened Toolchain Environment Validations ---
if ! command -v autoreconf >/dev/null 2>&1; then
    echo "Error: 'autoconf' and 'autoreconf' are required to build this package." >&2
    exit 1
fi

if ! command -v automake >/dev/null 2>&1; then
    echo "Error: 'automake' is required to build this package." >&2
    exit 1
fi

# Ensure macro storage root target exists before generating build files
mkdir -p m4

# Clean up any residual configuration state caches left behind by previous builds
rm -rf autom4te.cache config.status

# --- Primary Bootstrap Generation Pass ---
echo "Running autoreconf to generate build system configurations..."
# * Natively detects IT_PROG_INTLTOOL inside configure.ac and executes 
#   intltoolize internally in the mathematically correct order!
autoreconf --force --install --verbose

# Clear tracking caches out of the local source workspace root
rm -rf autom4te.cache

# --- Optional Automation Execution Hook ---
if test -n "$DOCONFIGURE"; then
    echo "Executing configure script..."
    ./configure "$@"
else
    echo "Build environment setup complete. You can now run './configure'."
fi

cd "$OLD_DIR"