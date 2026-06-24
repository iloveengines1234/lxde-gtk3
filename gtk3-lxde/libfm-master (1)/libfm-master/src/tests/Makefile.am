include $(top_srcdir)/Makefile.decl

NULL=

# Modernized CPPFLAGS—removed the compilation-breaking strict function declaration flag
AM_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/base \
	-I$(top_srcdir)/src/extra \
	-I$(top_srcdir)/src/actions \
	-I$(top_srcdir)/src/job \
	$(GIO_CFLAGS) \
	$(NULL)

noinst_PROGRAMS = $(TEST_PROGS) file-search-cli-demo

TEST_PROGS += fm-path
fm_path_SOURCES = test-fm-path.c
fm_path_LDADD= \
	$(top_builddir)/src/libfm.la \
	$(GIO_LIBS) \
	$(NULL)

# Integrated your specialized demo setup rules and local build flags
file_search_cli_demo_SOURCES = libfm-file-search-cli-demo.c
file_search_cli_demo_CFLAGS = $(GIO_CFLAGS) -I$(top_srcdir)/src
file_search_cli_demo_LDADD = \
	$(top_builddir)/src/libfm.la \
	$(GIO_LIBS) \
	$(NULL)