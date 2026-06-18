#!/bin/sh
# Updated by iloveengines1234 to support modern autotools and GTK 3 compilation environments

set -e

# Ensure the script runs from the repository root directory
cd "$(dirname "$0")"

echo "Checking for mandatory build dependencies..."
if ! command -v autoreconf >/dev/null 2>&1; then
    echo "Error: 'autoconf' and 'automake' utilities are required to build this project." >&2
    exit 1
fi

if ! command -v intltoolize >/dev/null 2>&1; then
    echo "Error: 'intltool' is required to process translation frameworks." >&2
    exit 1
fi

echo "Running intltoolize..."
intltoolize --force --copy --automake

echo "Bootstrapping build configuration via autoreconf..."
# Modern Fix: Added standard transient cache tracking cleanups directly inside the
# master automation sequence to prevent stale configuration rules between project state swaps.
rm -rf autom4te.cache

autoreconf --force --install --verbose

echo "
Ready to build! Now run:
  ./configure --enable-maintainer-mode
  make
"