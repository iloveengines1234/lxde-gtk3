# Process this file with automake to produce Makefile.in
# Updated by iloveengines1234 to port to gtk3

NULL =

man_in_files = lxrandr.xml
man_MANS = $(man_in_files:.xml=.1)

if ENABLE_REGENERATE_MAN
.xml.1:
	$(XSLTPROC) -o $@ -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

desktopdir = $(datadir)/applications
desktop_in_files = lxrandr.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Modernized translation merging rule sequence
@INTLTOOL_DESKTOP_RULE@

# Modern Fix: Shift from DISTCLEANFILES to CLEANFILES for build-generated artifacts,
# ensuring the built desktop shortcuts are correctly purged when resetting the source tree.
CLEANFILES = \
	$(desktop_DATA) \
	$(NULL)

# Crucial fix: Track the blueprint template (.desktop.in), NOT the generated output (.desktop)
# inside EXTRA_DIST so distributions are reproducible from a clean tarball.
EXTRA_DIST = \
	$(man_MANS) \
	$(man_in_files) \
	$(desktop_in_files) \
	$(NULL)