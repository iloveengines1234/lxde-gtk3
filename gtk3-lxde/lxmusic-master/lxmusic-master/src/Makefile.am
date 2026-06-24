## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(LIBNOTIFY_CFLAGS) \
	$(LXMUSIC_CFLAGS) \
	$(AYATANA_APPINDICATOR_CFLAGS)

bin_PROGRAMS = lxmusic

if ENABLE_NOTIFY
libnotify_sources = lxmusic-notify.c lxmusic-notify.h
else
libnotify_sources =
endif

lxmusic_SOURCES = \
	lxmusic.c \
	lxmusic-plugin-config.c \
	lxmusic-plugin-config.h \
	$(libnotify_sources) \
	utils.c utils.h

lxmusic_CFLAGS = $(LXMUSIC_CFLAGS) $(AYATANA_APPINDICATOR_CFLAGS) $(LIBNOTIFY_CFLAGS)

lxmusic_LDADD = $(LXMUSIC_LIBS) $(AYATANA_APPINDICATOR_LIBS) $(LIBNOTIFY_LIBS) $(INTLLIBS)