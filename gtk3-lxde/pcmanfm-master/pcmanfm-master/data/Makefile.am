NULL=

# UI subdir should be traversed first to generate layout files before building binaries
SUBDIRS = ui

dist_man_MANS = pcmanfm.1

desktopdir = $(datadir)/applications
desktop_in_files = \
	pcmanfm.desktop.in \
	pcmanfm-desktop-pref.desktop.in \
	$(NULL)

# Use standard modern Automake data targets instead of deprecated intltool macros
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Rule for handling modern desktop files via gettext if intltool is dropped
# If your configure.ac still relies entirely on intltool, you can keep @INTLTOOL_DESKTOP_RULE@
%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

defconfdir = $(sysconfdir)/xdg/pcmanfm/default
defconf_DATA = \
	pcmanfm.conf \
	$(NULL)

# Clean up generated files on 'make clean'
CLEANFILES = \
	$(desktop_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(desktop_in_files) \
	$(defconf_DATA) \
	$(NULL)