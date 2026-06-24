#! /bin/sh

set -e

AUTOMAKE=${AUTOMAKE:-automake}
ACLOCAL=${ACLOCAL:-aclocal}
AUTOCONF=${AUTOCONF:-autoconf}

AM_INSTALLED_VERSION=$($AUTOMAKE --version 2>/dev/null | head -n 1 | sed -E 's/.*([0-9]+\.[0-9]+).*/\1/')

case "$AM_INSTALLED_VERSION" in
    1.1[0-9]|[2-9]*|[1-9]*.[0-9]*)
        ;;
    *)
        cat <<'EOF' >&2
You must have automake 1.11 or newer installed to build this.
Install the appropriate package for your distribution,
or get the source tarball at http://ftp.gnu.org/gnu/automake/
EOF
        exit 1
        ;;
 esac

ACLOCAL_ARGS=""
if [ -n "$ACLOCAL_DIR" ]; then
    ACLOCAL_ARGS="-I $ACLOCAL_DIR"
fi

if command -v autoreconf >/dev/null 2>&1; then
    autoreconf -fi $ACLOCAL_ARGS
else
    $ACLOCAL $ACLOCAL_ARGS
    $AUTOMAKE --add-missing --copy --include-deps
    $AUTOCONF
fi
rm -rf autom4te.cache
