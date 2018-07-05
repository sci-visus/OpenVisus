
if (WIN32)

	file(GLOB debug_dlls   ${CMAKE_CURRENT_SOURCE_DIR}/win32/python36_d.dll)
	file(GLOB release_dlls ${CMAKE_CURRENT_SOURCE_DIR}/win32/python36.dll)

	Win32AddImportedLibrary(python 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/include 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/libs/python36_d.lib 
		${CMAKE_CURRENT_SOURCE_DIR}/win32/libs/python36.lib
		"${debug_dlls}" "${release_dlls}")
		
	set(PYTHON_EXECUTABLE ${CMAKE_CURRENT_SOURCE_DIR}/win32/python.exe)

else()
	MESSAGE(FATAL ERROR "VISUS_INTERNAL_PYTHON not supported")
endif()
