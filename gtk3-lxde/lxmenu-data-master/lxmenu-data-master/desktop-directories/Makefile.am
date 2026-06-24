NULL = 

directorydir = $(datadir)/desktop-directories

directory_in_files = \
	lxde-audio-video.directory.in \
	lxde-development.directory.in \
	lxde-education.directory.in \
	lxde-game.directory.in \
	lxde-graphics.directory.in \
	lxde-menu-applications.directory.in \
	lxde-network.directory.in \
	lxde-office.directory.in \
	lxde-other.directory.in \
	lxde-science.directory.in \
	lxde-settings.directory.in \
	lxde-system.directory.in \
	lxde-utility.directory.in \
	lxde-utility-accessibility.directory.in \
	$(NULL)

# Dynamically transform .directory.in specifications into localized runtime targets
directory_DATA = $(directory_in_files:.directory.in=.directory)

# Run standard intltool string merging rules over files
@INTLTOOL_DIRECTORY_RULE@

# FIXED: Removed generated $(directory_DATA) files from EXTRA_DIST. 
# Only ship the immutable template source files (.in) in the tarball.
EXTRA_DIST = $(directory_in_files)

# FIXED: Standardized target variable properties. Clean up localized build output 
# artifacts safely during typical 'make clean' routines.
CLEANFILES = $(directory_DATA)