#
# Find FreeImage
#
# Try to find FreeImage.
# This module defines the following variables:
# - FREEIMAGE_INCLUDE_DIRS
# - FREEIMAGE_LIBRARIES
# - FREEIMAGE_FOUND
#


find_path(FREEIMAGE_INCLUDE_DIRS NAMES FreeImage.h)

if (WIN32)
	FIND_LIBRARY(FREEIMAGE_LIBRARY_DEBUG   FreeImaged)	
	FIND_LIBRARY(FREEIMAGE_LIBRARY_RELEASE FreeImage)
	include(SelectLibraryConfigurations)
	select_library_configurations(FREEIMAGE)	
else()
	FIND_LIBRARY(FREEIMAGE_LIBRARIES NAMES freeimage)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeImage DEFAULT_MSG FREEIMAGE_INCLUDE_DIRS FREEIMAGE_LIBRARIES)
  
mark_as_advanced(FREEIMAGE_INCLUDE_DIRS FREEIMAGE_LIBRARIES)		

