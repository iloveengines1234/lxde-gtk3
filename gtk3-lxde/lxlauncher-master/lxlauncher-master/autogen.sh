#!/bin/sh
# Updated to support modern autotools and GTK 3.24.52 compilation environments

# Target binary assignments with environment fallbacks
AUTOMAKE=${AUTOMAKE:-automake}
AUTORECONF=${AUTORECONF:-autoreconf}

# Extract the major.minor version identifier cleanly
AM_INSTALLED_VERSION=$($AUTOMAKE --version | sed -e '2,$ d' -e 's/.* \([0-9]*\.[0-9]*\).*/\1/')

# Native support check matching modern version configurations (1.11 through 1.17+)
case "$AM_INSTALLED_VERSION" in
    1.1[1-9]|1.[2-9][0-9])
    ;;
    *)
    echo
    echo "Error: You must have automake >= 1.11 installed."
    echo "Detected version: $AM_INSTALLED_VERSION"
    echo "Please update your system distribution toolchains."
    exit 1
    ;;
esac

# Clean up any residual configuration state caches left behind by previous builds
rm -rf autom4te.cache config.status

# Echo commands as they run so developers can track build steps easily
set -x

# Modern Fix: Replace individual manual macro calls with a single unified autoreconf loop.
# --install: Automatically copies missing helper files (like compile, missing, install-sh).
# --force: Regenerates all configuration templates out of raw source entries cleanly.
# --verbose: Outputs clear progress logs down to the standard output terminal pipeline.
$AUTORECONF --force --install --verbose ${ACLOCAL_ARG}

# Clear tracing directories out of the local source directory workspace root
rm -rf autom4te.cache