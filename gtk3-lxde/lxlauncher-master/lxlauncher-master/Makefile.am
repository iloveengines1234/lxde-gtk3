## Process this file with automake to produce Makefile.in

NULL =

# --- Distribution Subdirectories Loop ---
SUBDIRS = src po

# --- System Configuration Path Mappings ---
confdir = $(sysconfdir)/xdg/lxlauncher
conf_in_files = settings.conf.in
conf_DATA = settings.conf

# --- Modern GTK3 Cascading CSS Themes ---
themedir = $(confdir)
theme_DATA = \
	gtk.css \
	$(NULL)

# --- Freedesktop Desktop Menu Assignments ---
menufiledir = $(sysconfdir)/xdg/menus
menufile_DATA = \
	data/lxlauncher-applications.menu \
	$(NULL)

# --- System Manual Pages Distribution ---
man_MANS = data/lxlauncher.1

# --- Freedesktop Target Directory Categories ---
directorydir = $(datadir)/desktop-directories
directory_in_files = \
	data/lxde-webmail.directory.in \
	data/lxde-work.directory.in \
	data/lxde-language.directory.in \
	data/lxde-learn.directory.in \
	data/lxde-play.directory.in \
	data/lxde-math.directory.in \
	$(NULL)

# Clean Suffix Mapping Translation Pattern Replacement for .directory targets
directory_DATA = $(directory_in_files:.directory.in=.directory)

# --- Localized Generation Translation Processing Patterns ---
# Safe processing fallbacks if native gettext macro hooks fail
@INTLTOOL_DIRECTORY_RULE@

# --- Explicit Runtime Configuration Rules ---
# CRITICAL FIX: Every recipe execution line below is explicitly indented using raw hard TABS.
settings.conf: settings.conf.in Makefile
	$(AM_V_GEN)$(SED) -e 's|@sysconfdir[@]|$(sysconfdir)|g' \
		-e 's|@datadir[@]|$(datadir)|g' $< > $@.tmp && \
	mv $@.tmp $@

# --- Distribution Package Source Manifest ---
EXTRA_DIST = \
	$(menufile_DATA) \
	$(conf_in_files) \
	$(theme_DATA) \
	$(directory_in_files) \
	$(directory_DATA) \
	data/lxlauncher.1 \
	autogen.sh \
	$(NULL)

# --- Build Cleanup Targets Execution Block ---
CLEANFILES = \
	$(conf_DATA) \
	$(directory_DATA) \
	settings.conf.tmp \
	$(NULL)