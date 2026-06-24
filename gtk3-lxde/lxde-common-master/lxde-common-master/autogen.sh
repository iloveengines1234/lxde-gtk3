#!/bin/sh
#
# autogen.sh - Modernized bootstrapper for lx-common-master
#

set -e # Exit immediately if any command in the script fails

# Target project root mapping
SRC_DIR=$(dirname "$0")
test -n "$SRC_DIR" || SRC_DIR=.

OLD_DIR=$(pwd)
cd "$SRC_DIR"

echo "Verifying modern build dependencies..."

# Ensure fundamental autotools programs exist in the current environment
if ! command -v automake >/dev/null 2>&1; then
    echo "CRITICAL: 'automake' is missing from the system environment."
    exit 1
fi

if ! command -v autoreconf >/dev/null 2>&1; then
    echo "CRITICAL: 'autoreconf' is missing. Please install the standard autoconf package."
    exit 1
fi

# Set up local system macro includes if provided by the environment
ACLOCAL_ARG=""
if [ -n "${ACLOCAL_DIR}" ]; then
    ACLOCAL_ARG="-I ${ACLOCAL_DIR}"
fi

echo "Bootstrapping structural configuration layers..."

# Turn on trace debugging output
set -x

# Handle standard internationalization template updates safely if present
if command -v intltoolize >/dev/null 2>&1; then
    intltoolize --automake --copy --force
fi

# Fixed: Replaced manual sequential tracking loops with a native, robust autoreconf call
autoreconf --force --install --verbose ${ACLOCAL_ARG}

# Clear out transient compilation data archives safely
rm -rf autom4te.cache

# Restore normal output visibility
set +x

echo "----------------------------------------------------"
echo "lx-common-master infrastructure successfully built!"
echo "----------------------------------------------------"

cd "$OLD_DIR"