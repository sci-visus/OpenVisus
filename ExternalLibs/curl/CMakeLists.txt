
# https://curl.haxx.se/download/curl-7.59.0.tar.gz
if (WIN32)
  add_definitions(-DPSAPI_VERSION=1)
  add_definitions(-DFD_SETSIZE=4096)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS )
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()

add_subdirectory(libressl)
add_subdirectory(lib)


