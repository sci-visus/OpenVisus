
if (WIN32)

	file(GLOB debug_dlls   ${CMAKE_CURRENT_SOURCE_DIR}/win32/debug/bin/*.dll)
	file(GLOB release_dlls ${CMAKE_CURRENT_SOURCE_DIR}/win32/bin/*.dll)

	Win32AddImportedLibrary(curl 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/include 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/debug/lib/libcurl.lib 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/libcurl.lib
		"${debug_dlls}" "${release_dlls}")
		
else()
	MESSAGE(FATAL ERROR "VISUS_INTERNAL_CURL not supported")
endif()	