# Finds liblz4.
#
# This module defines:
# LZ4_FOUND
# LZ4_INCLUDE_DIR
# LZ4_LIBRARY
#
find_path(LZ4_INCLUDE_DIR lz4.h)

if (WIN32)
	find_library(LZ4_LIBRARY_DEBUG   NAMES lz4d)
	find_library(LZ4_LIBRARY_RELEASE NAMES lz4)
	include(SelectLibraryConfigurations)
	select_library_configurations(LZ4)
ELSE()
	find_library(LZ4_LIBRARY NAMES lz4)
endif()

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
	LZ4 DEFAULT_MSG
	LZ4_LIBRARY LZ4_INCLUDE_DIR)

mark_as_advanced(LZ4_INCLUDE_DIR LZ4_LIBRARY)