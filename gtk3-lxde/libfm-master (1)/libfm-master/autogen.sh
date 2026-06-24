#!/bin/sh
#
# autogen.sh - Modernized GNU Autotools project bootstrapper
#

set -e # Terminate execution immediately if any command returns a failure code

# Safely extract directory context mapping targeting the project root
SRC_DIR=$(dirname "$0")
test -n "$SRC_DIR" || SRC_DIR=.

OLD_DIR=$(pwd)
cd "$SRC_DIR"

echo "Checking system dependency footprints..."

# Fixed: Replaced deprecated non-POSIX 'which' utility with standard 'command -v'
if ! command -v gtkdocize >/dev/null 2>&1; then
    echo "CRITICAL: 'gtkdocize' could not be found."
    echo "You must install 'gtk-doc' to bootstrap this package ecosystem."
    echo "Documentation: https://developer.gnome.org/gtk-doc/"
    exit 1
fi

if ! command -v automake >/dev/null 2>&1; then
    echo "CRITICAL: 'automake' is not available in the current PATH environment."
    exit 1
fi

# Ensure mandatory layout structures exist before generating macros
mkdir -p m4

# Inject custom system macro directory pointers if exported by the build system
ACLOCAL_ARG=""
if [ -n "${ACLOCAL_DIR}" ]; then
    ACLOCAL_ARG="-I ${ACLOCAL_DIR}"
fi

echo "Generating build configuration interface files..."

# Enable strict echo visibility debugging tracking
set -x

# Execute specialized GTK API documentation staging updates
gtkdocize --copy

# Prepare translation framework file templates if supported
if command -v intltoolize >/dev/null 2>&1; then
    intltoolize --automake --copy --force
fi

# Fixed: Replaced the brittle sequential step chain with a unified 'autoreconf' layer.
# This automatically handles internal cross-dependencies safely.
autoreconf --force --install --verbose ${ACLOCAL_ARG}

# Clear dynamic compilation trace artifacts out cleanly
rm -rf autom4te.cache

# Turn off verbose tracking flags for interactive confirmation
set +x

echo "--------------------------------------------------------"
echo "Bootstrap complete. Codebase is ready for compilation."
echo "--------------------------------------------------------"

# Handle dynamic configuration execution transitions if explicitly checked
if [ -n "$DOCONFIGURE" ]; then
    echo "Executing configure layer initialization: ./configure $*"
    ./configure "$@"
fi

cd "$OLD_DIR"