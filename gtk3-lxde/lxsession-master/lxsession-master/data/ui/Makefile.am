# Modern Fix: Keep track of GtkBuilder assets cleanly using variables
NULL =

# Core UI resource files to bundle
UI_FILES = \
	lxpolkit.ui \
	lxsession-default-apps.ui \
	lxsession-edit.ui \
	$(NULL)

# Modern Fix: Shift from raw filesystem distribution to a compiled GResource structure.
# This prevents asset path mismatches and matches modern GTK 3 deployment specs.
glib_resources_xml = lxsession-resources.gresource.xml
glib_resources_c = lxsession-resources.c

# Rule to automatically generate the resources C binding source code at build time
$(glib_resources_c): $(glib_resources_xml) $(UI_FILES)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

# Include the generated source context directly into your main application target
# (Note: Double check that 'lxsession_SOURCES' matches the precise binary name used elsewhere in your file)
lxsession_SOURCES = \
	$(glib_resources_c) \
	main.c \
	$(NULL)

# Ensure the XML mapping blueprint and all source UI files are correctly packed into the source tarball
EXTRA_DIST = \
	$(glib_resources_xml) \
	$(UI_FILES) \
	$(NULL)

# Clean up build-time generated binary files when running 'make clean'
CLEANFILES = \
	$(glib_resources_c) \
	$(NULL)