
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/ExternalLib)

if(WIN32)
	add_compile_options("/W0")
else()
	add_compile_options("-w")
	add_compile_options(-fvisibility=hidden) # this fixes the problem of static symbols conflicting with dynamic lib
endif()

add_subdirectory(cryptlite)
add_subdirectory(dtl)
add_subdirectory(lz4)
add_subdirectory(tinyxml)
add_subdirectory(zlib)
add_subdirectory(zfp)

if (VISUS_NET)
	add_subdirectory(libressl)
	add_subdirectory(curl)
endif()

if (VISUS_IMAGE)
	add_subdirectory(FreeImage)
endif()
