## Process this file with automake to produce Makefile.in

# Global compilation flags found by configure
AM_CPPFLAGS = \
	$(all_includes) \
	$(X11_CFLAGS) \
	$(GTK_CFLAGS) \
	$(ADDITIONAL_FLAGS) \
	-I$(top_srcdir) \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

# Binary program targets
bin_PROGRAMS = gpicview
gpicview_SOURCES = \
	gpicview.c \
	image-list.c \
	main-win.c \
	image-view.c \
	working-area.c \
	ptk-menu.c \
	file-dlgs.c \
	jpeg-tran.c \
	tran-supp.c \
	pref.c \
	jhead.c \
	exif.c \
	jpgfile.c 

# Library link dependencies
gpicview_LDADD = $(X11_LIBS) $(GTK_LIBS) -lm

# Development header distributions
noinst_HEADERS = \
	glib-mem.h \
	image-list.h \
	main-win.h \
	image-view.h \
	working-area.h \
	ptk-menu.h \
	file-dlgs.h \
	jpeg-tran.h \
	tran-supp.h \
	pref.h \
	jhead.h

# Internal utility targets
noinst_PROGRAMS = xml-purge
xml_purge_SOURCES = xml-purge.c
xml_purge_CFLAGS = $(X11_CFLAGS)
xml_purge_LDADD = $(X11_LIBS)