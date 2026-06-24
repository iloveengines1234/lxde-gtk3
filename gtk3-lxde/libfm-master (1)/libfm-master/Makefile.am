include $(top_srcdir)/Makefile.decl

NULL=

# Safely initialize global distribution variables at the top level
# This fixes the fatal modern Automake "append before definition" compilation crash.
EXTRA_DIST = $(NULL)

DIST_SUBDIRS = src po data

# A little trick against gtk-doc is here
SUBDIRS_DOCS = docs

EXTRA_DIST_DOCS = \
	gtk-doc.make \
	docs/Makefile.am \
	docs/Makefile.in \
	docs/reference/Makefile.am \
	docs/reference/Makefile.in \
	docs/reference/libfm/Makefile.am \
	docs/reference/libfm/Makefile.in \
	docs/reference/libfm/libfm-docs.xml \
	docs/reference/libfm/libfm-sections.txt \
	docs/reference/libfm/version.xml.in \
	$(NULL)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libfm-extra.pc

if EXTRALIB_ONLY

SUBDIRS = src

else

SUBDIRS = $(DIST_SUBDIRS)

# Fixed: Appending is now completely safe since EXTRA_DIST was initialized above
EXTRA_DIST += \
	libfm.pc.in \
	libfm-extra.pc.in \
	libfm-gtk3.pc.in \
	po/libfm.pot \
	$(NULL)

if ENABLE_GTK_DOC
DIST_SUBDIRS += $(SUBDIRS_DOCS)
SUBDIRS += $(SUBDIRS_DOCS)
else
EXTRA_DIST += $(EXTRA_DIST_DOCS)
endif

# Only map and expose the explicit GTK3 package configuration asset
pkgconfig_DATA += \
	libfm.pc \
	@LIBFM_GTK_PC@ \
	$(NULL)

endif

# Standard flags passed to make distcheck to validate complete builds
DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc