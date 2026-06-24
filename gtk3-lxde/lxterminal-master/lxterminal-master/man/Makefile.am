# Define manual targets for installation paths
man_MANS = \
	lxterminal.1

# Source XML definition components
man_XMANS = \
	lxterminal.xml

# Resource configurations distributed to the release package tarball
EXTRA_DIST = \
	$(man_MANS) \
	$(man_XMANS)

# Clean rules handling build artifacts
CLEANFILES =

# Conditional block handled by configure.ac check flags
if ENABLE_REGENERATE_MAN

# Rule to transform docbook xml into standard UNIX manual format
lxterminal.1: lxterminal.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

CLEANFILES += $(man_MANS)

endif