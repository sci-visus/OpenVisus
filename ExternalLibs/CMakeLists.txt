
add_subdirectory(cryptlite)
add_subdirectory(dtl)
add_subdirectory(lz4)
add_subdirectory(tinyxml)
add_subdirectory(zlib)

if (VISUS_NET)		
	add_subdirectory(libressl)
	add_subdirectory(curl)
endif() 

if (VISUS_IMAGE)
	add_subdirectory(FreeImage)
endif()
