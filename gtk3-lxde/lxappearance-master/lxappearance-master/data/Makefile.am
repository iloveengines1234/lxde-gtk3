NULL =

SUBDIRS = \
	ui \
	$(NULL)

pkgconfigdir   = $(libdir)/pkgconfig
pkgconfig_DATA = lxappearance.pc

# Desktop entry localization and generation
desktopdir = $(datadir)/applications
desktop_in_files = \
	lxappearance.desktop.in \
	$(NULL)
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Modern standard rule for processing desktop files via msgfmt/gettext
%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

CLEANFILES = \
	$(desktop_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(desktop_in_files) \
	$(NULL)