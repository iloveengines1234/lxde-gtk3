NULL =

# Standardize path configurations to use modern datadir conventions
uidir = $(datadir)/pcmanfm/ui

ui_SOURCES = \
	pref.glade \
	desktop-pref.glade \
	autorun.glade \
	connect.glade \
	$(NULL)

ui_in_files = \
	$(ui_SOURCES) \
	about.glade \
	$(NULL)

ui_DATA = $(ui_in_files:.glade=.ui)

# Ensure ALL generated .ui files are cleaned up, not just about.ui
CLEANFILES = \
	$(ui_DATA) \
	$(NULL)

EXTRA_DIST = \
	$(ui_in_files) \
	$(NULL)

# Modernized compilation rule with Automake Silent Rules support
.glade.ui:
	$(AM_V_GEN)sed 's///' < $< | sed ':a;N;$$!ba;s/ *\n *</</g' > $@