man_MANS = lxinput.1

EXTRA_DIST = \
	$(man_MANS) \
	lxinput.xml

CLEANFILES =

if ENABLE_REGENERATE_MAN

lxinput.1: lxinput.xml
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

CLEANFILES += $(man_MANS)

endif

