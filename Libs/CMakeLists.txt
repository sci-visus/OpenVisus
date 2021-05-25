
add_subdirectory(Kernel)

if (VISUS_GUI)
	add_subdirectory(Gui)
endif()

if (VISUS_PYTHON)
	add_subdirectory(swig)
endif()

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VisusWeaving/CMakeLists.txt")
	add_subdirectory(VisusWeaving)
endif()	
