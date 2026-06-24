#!/bin/sh
# ==============================================================================
# autogen.sh - GNU Autotools Bootstrap Configuration Script
# Component: lxappearance-obconf
# ==============================================================================

set -e # Terminate immediately if any pipeline instruction drops a failure state

# 1. Ensure essential tool infrastructure is present on the path
for tool in autoreconf automake autoconf pkg-config intltoolize libtoolize; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: Required development utility '$tool' was not found on your system PATH." >&2
        echo "Please install the autotools, pkg-config, and intltool packages for your distribution." >&2
        exit 1
    fi
done

# 2. Extract and validate Automake versions safely using structural comparison logic
AUTOMAKE=${AUTOMAKE:-automake}
AM_VERSION_RAW=$($AUTOMAKE --version | head -n 1 | sed -E 's/.* ([0-9]+\.[0-9]+).*/\1/')

# Extract major and minor version numbers
AM_MAJOR=$(echo "$AM_VERSION_RAW" | cut -d. -f1)
AM_MINOR=$(echo "$AM_VERSION_RAW" | cut -d. -f2)

# Verify minimum threshold: Automake >= 1.11
if [ "$AM_MAJOR" -lt 1 ] || { [ "$AM_MAJOR" -eq 1 ] && [ "$AM_MINOR" -lt 11 ]; }; then
    echo "Error: Detected Automake version $AM_VERSION_RAW. You must have automake >= 1.11 installed." >&2
    exit 1
fi

echo "Bootstrapping lxappearance-obconf configuration layout (Automake $AM_VERSION_RAW)..."

# 3. Clean environment workspace caches before regenerating configuration schemas
rm -rf autom4te.cache config.status

# 4. Handle supplementary macro directory lookups dynamically if configured
ACLOCAL_FLAGS="${ACLOCAL_FLAGS}"
if [ -n "${ACLOCAL_DIR}" ]; then
    ACLOCAL_FLAGS="${ACLOCAL_FLAGS} -I ${ACLOCAL_DIR}"
fi

# Ensure localized project macro directory is included in the scan context
if [ -d "m4" ]; then
    ACLOCAL_FLAGS="${ACLOCAL_FLAGS} -I m4"
fi

# Export to environment so autoreconf passes variables down to background aclocal steps
export ACLOCAL_FLAGS

# 5. Let autoreconf seamlessly orchestrate initialization tools under the hood
echo "Executing autoreconf compilation chain..."
set -x

# Added explicit macro pass flags into autoreconf to coordinate clean builds
autoreconf --force --install --verbose

# 6. Clean up configuration caching structures
{ set +x; } 2>/dev/null
rm -rf autom4te.cache
echo "Bootstrap completed successfully. You may now execute './configure'."