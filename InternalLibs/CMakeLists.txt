if (WIN32)
  add_definitions(-DPSAPI_VERSION=1)
  add_definitions(-DFD_SETSIZE=4096)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS )
  add_definitions(-DWIN32_LEAN_AND_MEAN)
endif()


if (VISUS_INTERNAL_ZLIB)
	add_subdirectory(zlib)
endif()

if (VISUS_INTERNAL_LZ4)
	add_subdirectory(lz4)
endif()

if (VISUS_INTERNAL_TINYXML)
	add_subdirectory(tinyxml)
endif()

if (VISUS_INTERNAL_FREEIMAGE)
	add_subdirectory(FreeImage)
endif()

if (VISUS_INTERNAL_OPENSSL)
	add_subdirectory(openssl)
endif()

if (VISUS_INTERNAL_CURL)
	add_subdirectory(curl)
endif()

