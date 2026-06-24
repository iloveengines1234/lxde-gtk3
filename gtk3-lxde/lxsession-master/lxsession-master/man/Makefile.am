man_MANS = \
	lxsession.1 \
	lxsession-logout.1 \
	lxclipboard.1 \
	lxlock.1 \
	lxsession-default.1 \
	lxsession-default-terminal.1 \
	lxsession-default-apps.1 \
	lxsettings-daemon.1 \
	lxpolkit.1 \
	lxsession-edit.1 \
	lxsession-db.1 \
	lxsession-xdg-autostart.1

# Track EVERY xml source file so they are bundled into the release tarball
man_XMLS = \
	lxsession.xml \
	lxsession-logout.xml \
	lxclipboard.xml \
	lxlock.xml \
	lxsession-default.xml \
	lxsession-default-terminal.xml \
	lxsession-default-apps.xml \
	lxsettings-daemon.xml \
	lxpolkit.xml \
	lxsession-edit.xml \
	lxsession-db.xml \
	lxsession-xdg-autostart.xml

EXTRA_DIST = \
	$(man_MANS) \
	$(man_XMLS)

# Always initialize CLEANFILES outside of conditional blocks
CLEANFILES =

if ENABLE_REGENERATE_MAN

# Declare the suffix extensions so Automake registers the rule transformation
SUFFIXES = .xml .1

# Universally portable suffix rule to compile ANY .xml file into its corresponding .1 man page
.xml.1:
	$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

# Safely append the generated outputs to the cleanup queue
CLEANFILES += $(man_MANS)

endif