## Process this file with automake to produce Makefile.in

# The name of the documentation module
DOC_MODULE = libmenu-cache

# The top-level SGML/XML documentation entry file
DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.sgml

# Source paths scanned for inline code documentation blocks
DOC_SOURCE_DIR = $(top_srcdir)/libmenu-cache

# Compilation options parsed to the individual gtk-doc execution engines
SCANGOBJ_OPTIONS = 
SCAN_OPTIONS = --rebuild-sections --rebuild-types --deprecated-guards="G_DISABLE_DEPRECATED"
MKDB_OPTIONS = --xml-mode --output-format=xml
MKHTML_OPTIONS = 
FIXXREF_OPTIONS = 

# File globs monitored for rebuilding documentation structures
HFILE_GLOB = $(top_srcdir)/libmenu-cache/*.h
CFILE_GLOB = $(top_srcdir)/libmenu-cache/*.c

# Files or subdirectories excluded from documentation parsing passes
IGNORE_HFILES = version.h

# Optional static asset listings
EXTRA_HFILES = 
HTML_IMAGES = 
content_files = 
expand_content_files = 

# CFLAGS and LDFLAGS needed to compile scangobj against your internal library targets
GTKDOC_CFLAGS = -I$(top_srcdir) $(GLIB_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/libmenu-cache/libmenu-cache.la $(GLIB_LIBS)

# Pull in standard system rules copied dynamically via gtkdocize
include $(top_srcdir)/gtk-doc.make

# Supplementary distribution assets tracking configurations
EXTRA_DIST +=

# Transient file arrays cleanly purged during distribution workspace resets
DISTCLEANFILES = $(DOC_MODULE).types $(DOC_MODULE)-sections.txt

# Maintain robust parallel validation rules
$(DOC_MAIN_SGML_FILE): sgml-build.stamp

# Sanity testing environments configuration
if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) && \
	DOC_MODULE=$(DOC_MODULE) DOC_MAIN_SGML_FILE=$(DOC_MAIN_SGML_FILE) \
	SRCDIR=$(abs_srcdir) BUILDDIR=$(abs_builddir)
endif

-include $(top_srcdir)/git.mk