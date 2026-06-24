# List of manual pages to be cleanly packaged and distributed inside source tarballs
dist_man_MANS = \
	lxde-logout.1 \
	startlxde.1 \
	openbox-lxde.1 \
	$(NULL)

# Source XML blueprints required for rebuilding the documents
EXTRA_DIST = \
	openbox-lxde.xml \
	$(NULL)

if ENABLE_REGENERATE_MAN

# Maintainer Rule: Compiles the source XML into standard man layouts using xsltproc
openbox-lxde.1: openbox-lxde.xml
	$(XSLTPROC) --nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

# Ensure locally generated assets are wiped during clean loops
CLEANFILES = \
	openbox-lxde.1 \
	$(NULL)

endif