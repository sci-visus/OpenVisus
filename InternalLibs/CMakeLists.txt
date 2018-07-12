
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

if (VISUS_INTERNAL_PYTHON)
	add_subdirectory(python)
endif()
