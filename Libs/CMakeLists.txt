
add_subdirectory(Kernel)


add_subdirectory(Db)

if (VISUS_DATAFLOW)

	add_subdirectory(Dataflow)
	add_subdirectory(Nodes)

	if (VISUS_GUI)
		add_subdirectory(Gui)
	endif()
	
endif()

if (VISUS_PYTHON)
	add_subdirectory(swig)
endif()


if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VisusWeaving/CMakeLists.txt")
	add_subdirectory(VisusWeaving)
endif()	
