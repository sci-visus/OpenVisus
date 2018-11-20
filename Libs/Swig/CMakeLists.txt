AddSwigLibrary(NonGuiOpenVisus ${CMAKE_CURRENT_SOURCE_DIR}/NonGuiOpenVisus.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include")
target_link_libraries(${SWIG_MODULE_NonGuiOpenVisus_REAL_NAME} PUBLIC VisusIdx VisusNodes VisusDataflow VisusDb VisusKernel)

if (VISUS_GUI)
	AddSwigLibrary(GuiOpenVisus ${CMAKE_CURRENT_SOURCE_DIR}/GuiOpenVisus.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../AppKit/include")
	target_link_libraries(${SWIG_MODULE_GuiOpenVisus_REAL_NAME} PUBLIC VisusAppKit VisusGuiNodes VisusGui VisusIdx VisusNodes VisusDataflow VisusDb VisusKernel)
endif()


