SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/ExternalLib)

if (MSVC)
	add_compile_options("/W0")
else()
	add_compile_options("-w")
	add_compile_options(-fvisibility=hidden) # this fixes the problem of static symbols conflicting with dynamic lib
endif()

if (VISUS_NET)
	add_subdirectory(libressl-3.6.1)
	add_subdirectory(curl-7.86.0) 
endif()

if (VISUS_IMAGE)
	add_subdirectory(FreeImage)
endif()
