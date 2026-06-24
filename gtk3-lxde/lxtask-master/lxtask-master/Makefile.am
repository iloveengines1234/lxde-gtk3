# Process this file with automake to produce Makefile.in
# Updated by iloveengines1234 to port completely to GTK 3 / Modern Autotools

ACLOCAL_AMFLAGS = -I .

SUBDIRS = po src

desktopdir = $(datadir)/applications
desktop_in_files = data/lxtask.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Modern Fix: Swapped out deprecated @INTLTOOL_DESKTOP_RULE@ marker syntax 
# for standard intltool rule inclusion to conform with contemporary translation workflows.
$(desktop_DATA): $(desktop_in_files)
	$(AM_V_GEN)$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

man_MANS = data/lxtask.1

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST = \
	$(desktop_in_files) \
	$(man_MANS)

DISTCLEANFILES = \
	$(desktop_DATA)# Process this file with automake to produce Makefile.in
# Updated by iloveengines1234 to port completely to GTK 3 / Modern Autotools

ACLOCAL_AMFLAGS = -I .

SUBDIRS = po src

desktopdir = $(datadir)/applications
desktop_in_files = data/lxtask.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Modern Fix: Swapped out deprecated @INTLTOOL_DESKTOP_RULE@ marker syntax 
# for standard intltool rule inclusion to conform with contemporary translation workflows.
$(desktop_DATA): $(desktop_in_files)
	$(AM_V_GEN)$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

man_MANS = data/lxtask.1

distclean-local:
	rm -rf *.cache *~

rpm: dist
	rpmbuild -ta $(PACKAGE)-$(VERSION).tar.gz
	@rm -f $(PACKAGE)-$(VERSION).tar.gz

EXTRA_DIST = \
	$(desktop_in_files) \
	$(man_MANS)

DISTCLEANFILES = \
	$(desktop_DATA)