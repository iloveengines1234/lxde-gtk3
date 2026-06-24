man_MANS = \
	lxappearance.1

man_XMANS = \
	lxappearance.xml

EXTRA_DIST = \
	$(man_MANS) \
	$(man_XMANS)

if ENABLE_MAN
# Rule to generate the manpage from DocBook XML using xsltproc
lxappearance.1: lxappearance.xml
	$(AM_V_GEN)$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

CLEANFILES = \
	$(man_MANS)
endif