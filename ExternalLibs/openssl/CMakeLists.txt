if (WIN32)

	file(GLOB debug_dlls   ${CMAKE_CURRENT_SOURCE_DIR}/win32/debug/bin/*.dll)
	file(GLOB release_dlls ${CMAKE_CURRENT_SOURCE_DIR}/win32/bin/*.dll)
	
	Win32AddImportedLibrary(crypto 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/include 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/debug/lib/libeay32.lib 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/libeay32.lib
		"${debug_dlls}" "${release_dlls}")
		
	Win32AddImportedLibrary(ssl    
		${CMAKE_CURRENT_SOURCE_DIR}/win32/include 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/debug/lib/ssleay32.lib 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/lib/ssleay32.lib
		"" "")

else()
	MESSAGE(FATAL ERROR "VISUS_INTERNAL_OPENSSL not supported")
endif()

