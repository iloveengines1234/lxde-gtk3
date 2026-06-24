## Process this file with automake to produce Makefile.in

NULL =

# --- Standard XDG Menu Configuration Directory Path ---
# Modern Fix: Default to standard data directory paths to honor modern 
# distro packaging specs, allowing clean system administrator overrides.
menudir = $(datadir)/menus

# --- Distribution Menu Data Architecture ---
# CRITICAL FIX: Prepended 'dist_' to guarantee Automake tracks this file 
# inside the source tarball, preventing VPATH/out-of-tree build failures.
dist_menu_DATA = \
	lxde-applications.menu \
	$(NULL)

# --- Build Cleanup Targets Execution Block ---
CLEANFILES = \
	$(NULL)