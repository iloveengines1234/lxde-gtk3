#!/bin/sh
# Modernized autogen.sh for lxhotkey

set -e

# Determine source directory context safely
SRC_DIR=$(dirname "$0")
test -n "$SRC_DIR" || SRC_DIR=.

OLD_DIR=$(pwd)
cd "$SRC_DIR"

# Ensure macro storage root target exists before generating build configurations
mkdir -p m4

# Clean up any residual configuration state caches left behind by previous builds
rm -rf autom4te.cache config.status

# Echo commands as they run so developers can track build steps easily
set -x

# Modern Fix: Replace individual manual macro loops with a single unified autoreconf pass.
# --install: Automatically copies missing helper files (like compile, missing, install-sh).
# --force: Regenerates all configuration templates out of raw source entries cleanly.
# --verbose: Outputs clear progress logs down to the standard output terminal pipeline.
autoreconf --force --install --verbose ${ACLOCAL_ARG}

# Clear tracing directories out of the local source directory workspace root
rm -rf autom4te.cache

echo ""
echo "Build environment setup complete. You can now run './configure'."

cd "$OLD_DIR"