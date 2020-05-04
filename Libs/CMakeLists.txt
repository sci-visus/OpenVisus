
add_subdirectory(Kernel)
add_subdirectory(XIdx)
add_subdirectory(Db)
add_subdirectory(Dataflow)
add_subdirectory(Nodes)

if (VISUS_GUI)
	add_subdirectory(Gui)
endif()
  
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Slam/CMakeLists.txt)
	message(STATUS "Enabled VisusSlam")
	add_subdirectory(Slam)
endif()

