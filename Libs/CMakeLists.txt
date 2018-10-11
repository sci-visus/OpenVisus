
add_subdirectory(Kernel)
add_subdirectory(Dataflow)
add_subdirectory(Db)
add_subdirectory(Idx)
add_subdirectory(Nodes)

if (VISUS_GUI)
  add_subdirectory(Gui)
  add_subdirectory(GuiNodes)
  add_subdirectory(AppKit)
endif()

AddSwigLibrary(OpenVisusPy ${CMAKE_CURRENT_SOURCE_DIR}/OpenVisusPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/Idx/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/Gui/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/Nodes/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/GuiNodes/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/AppKit/include")

target_link_libraries(_OpenVisusPy PUBLIC VisusIdx VisusNodes VisusDataflow VisusDb VisusKernel)

if (VISUS_GUI)
	target_link_libraries(_OpenVisusPy PUBLIC VisusAppKit VisusGuiNodes VisusGui)
endif()




