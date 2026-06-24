NULL =

AM_CPPFLAGS = \
    -DPACKAGE_DATA_DIR=\"$(datadir)/pcmanfm\" \
    -DPACKAGE_UI_DIR=\"$(datadir)/pcmanfm/ui\" \
    -DPACKAGE_MODULES_DIR=\"$(libdir)/pcmanfm\" \
    -DPACKAGE_LOCALE_DIR=\"$(datadir)/locale\" \
    $(GTK_CFLAGS) \
    $(FM_CFLAGS)

bin_PROGRAMS = pcmanfm

pcmanfm_SOURCES = \
    pcmanfm.c \
    app-config.c \
    main-win.c \
    tab-page.c \
    desktop.c \
    volume-manager.c \
    pref.c \
    single-inst.c \
    connect-server.c \
    $(NULL)

# These files are included, not compiled
EXTRA_DIST = \
    main-win-ui.c \
    desktop-ui.c \
    pcmanfm.h \
    app-config.h \
    main-win.h \
    tab-page.h \
    desktop.h \
    volume-manager.h \
    pref.h \
    single-inst.h \
    connect-server.h \
    gseal-gtk-compat.h \
    $(NULL)

include_HEADERS = pcmanfm-modules.h

pcmanfm_CFLAGS = \
    $(GTK_CFLAGS) \
    $(FM_CFLAGS) \
    -Wall \
    -Wextra \
    -Wno-deprecated-declarations \
    -Werror=implicit-function-declaration \
    $(NULL)

pcmanfm_LDADD = \
    $(GTK_LIBS) \
    $(FM_LIBS) \
    $(NULL)

# Install modules directory
install-exec-local:
    $(MKDIR_P) "$(DESTDIR)$(libdir)/pcmanfm"

uninstall-local:
    -rmdir "$(DESTDIR)$(libdir)/pcmanfm"
