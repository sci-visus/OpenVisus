
add_subdirectory(Kernel)
add_subdirectory(XIdx)
add_subdirectory(Db)
add_subdirectory(Dataflow)
add_subdirectory(Nodes)

if (VISUS_GUI)
	add_subdirectory(Gui)
endif()
  
if (VISUS_SLAM)
	add_subdirectory(Slam)
endif()

