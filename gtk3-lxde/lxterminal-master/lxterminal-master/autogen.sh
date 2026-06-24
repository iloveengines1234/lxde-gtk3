#!/bin/sh
# autogen.sh - Bootstraps the autotools configuration engine
#
# Copyright (C) 2016 Andriy Grytsenko <andrej@rep.kiev.ua>
# Copyright (C) 2026 LXDE Project Maintainers
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

set -e  # Terminate execution immediately if any command fails

AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOHEADER=${AUTOHEADER:-autoheader}
AUTOCONF=${AUTOCONF:-autoconf}
INTLTOOLIZE=${INTLTOOLIZE:-intltoolize}

# Verify that the system has an acceptable Automake installation
if ! command -v "$AUTOMAKE" >/dev/null 2>&1; then
    echo "Error: '$AUTOMAKE' is not installed or available in your PATH." >&2
    exit 1
fi

AM_INSTALLED_VERSION=$($AUTOMAKE --version | head -n 1 | sed 's/.* //')
REQUIRED_VERSION="1.11"

# Compare version numbers using a sort check safe for cross-platform systems
VERSION_CHECK=$(printf '%s\n%s\n' "$REQUIRED_VERSION" "$AM_INSTALLED_VERSION" | sort -V | head -n 1)

if [ "$VERSION_CHECK" != "$REQUIRED_VERSION" ] && [ "$AM_INSTALLED_VERSION" != "$REQUIRED_VERSION" ]; then
    echo "Error: You must have automake >= $REQUIRED_VERSION installed." >&2
    echo "Current detected version is: $AM_INSTALLED_VERSION" >&2
    echo "Please update your system packages or compile from source at http://ftp.gnu.org/gnu/automake/" >&2
    exit 1
fi

# Build macro search arguments dynamically
ACLOCAL_ARG=""
if [ -n "${ACLOCAL_DIR}" ]; then
    ACLOCAL_ARG="-I ${ACLOCAL_DIR}"
fi

# Ensure common default tracking paths are queried for auxiliary macro definitions
if [ -d /usr/share/aclocal ]; then
    ACLOCAL_ARG="$ACLOCAL_ARG -I /usr/share/aclocal"
fi
if [ -d /usr/local/share/aclocal ]; then
    ACLOCAL_ARG="$ACLOCAL_ARG -I /usr/local/share/aclocal"
fi

# Clean out old artifacts before bootstrapping
rm -rf autom4te.cache config.cache

set -x  # Print commands to stderr as they are executed

# 1. Initialize translation definitions
$INTLTOOLIZE --automake --copy --force

# 2. Collect local and global configuration macros
$ACLOCAL $ACLOCAL_ARG

# 3. Build templates for configuration metrics
$AUTOHEADER --force

# 4. Bind makefile definitions and add missing scaffolding scripts
$AUTOMAKE --add-missing --copy --foreign

# 5. Compile configure scripts from the configured macros
$AUTOCONF

set +x  # Disable command tracing

echo ""
echo "Bootstrapping completed successfully."
echo "You can now run './configure' with your preferred configuration switches."
echo ""