# Traversal tree mapping the build execution ordering sequence
SUBDIRS = src po data man

# Configure strict regression build environments for 'make distcheck' runs
DISTCHECK_CONFIGURE_FLAGS = --enable-man --enable-gtk3

# Auxiliary files and structural distribution blueprints packed into source tarballs
EXTRA_DIST = \
	autogen.sh \
	acinclude.m4 \
	intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	$(NULL)

# Automation clean hooks to eliminate intltool conversion artifacts
DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache \
	$(NULL)

# Ensure local m4 macros directory states are purged correctly on deep maintenance
clean-local:
	rm -rf autom4te.cache