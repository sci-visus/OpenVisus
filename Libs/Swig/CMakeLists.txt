
AddSwigLibrary(VisusKernelPy VisusKernel ${CMAKE_CURRENT_SOURCE_DIR}/VisusKernelPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")
	
AddSwigLibrary(VisusDataflowPy VisusDataflow ${CMAKE_CURRENT_SOURCE_DIR}/VisusDataflowPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include")
	
AddSwigLibrary(VisusDbPy VisusDb ${CMAKE_CURRENT_SOURCE_DIR}/VisusDbPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include")
	
AddSwigLibrary(VisusIdxPy VisusIdx ${CMAKE_CURRENT_SOURCE_DIR}/VisusIdxPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include")		

AddSwigLibrary(VisusNodesPy VisusNodes ${CMAKE_CURRENT_SOURCE_DIR}/VisusNodesPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include")
	
if (VISUS_GUI)

	AddSwigLibrary(VisusGuiPy VisusGui ${CMAKE_CURRENT_SOURCE_DIR}/VisusGuiPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include")
	
	AddSwigLibrary(VisusGuiNodesPy VisusGuiNodes ${CMAKE_CURRENT_SOURCE_DIR}/VisusGuiNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include")
	
	AddSwigLibrary(VisusAppKitPy VisusAppKit ${CMAKE_CURRENT_SOURCE_DIR}/VisusAppKitPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../AppKit/include")

endif()


