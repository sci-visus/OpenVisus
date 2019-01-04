#  APACHE_FOUND - System has APACHE
#  APACHE_INCLUDE_DIR - The APACHE include directory
#
#  APACHE_DIR
#   setting this enables search for apache libraries / headers in this location

#
# Include directories
#
find_path(APACHE_INCLUDE_DIR
          NAMES httpd.h 
          PATH_SUFFIXES httpd apache apache2 apache22 apache24
          HINTS ${APACHE_DIR} ${APACHE_DIR}/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(APACHE DEFAULT_MSG APACHE_INCLUDE_DIR)
mark_as_advanced(APACHE_INCLUDE_DIR)

