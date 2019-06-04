
if (1)
	AddSwigLibrary(VisusKernel ${CMAKE_CURRENT_SOURCE_DIR}/VisusKernelPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")
endif()

if (VISUS_XIDX)
	AddSwigLibrary(VisusXIdx ${CMAKE_CURRENT_SOURCE_DIR}/VisusXIdx.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../XIdx/include")
endif()

if (VISUS_DATAFLOW)
	AddSwigLibrary(VisusDataflow ${CMAKE_CURRENT_SOURCE_DIR}/VisusDataflowPy.i
		"-I${CMAKE_CURRENT_SOURCE_DIR}" 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include")
endif()

if (VISUS_DB)
	AddSwigLibrary(VisusDb ${CMAKE_CURRENT_SOURCE_DIR}/VisusDbPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include")
endif()

if (VISUS_IDX)
	AddSwigLibrary(VisusIdx ${CMAKE_CURRENT_SOURCE_DIR}/VisusIdxPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include")
endif()

if (VISUS_NODES)
	AddSwigLibrary(VisusNodes ${CMAKE_CURRENT_SOURCE_DIR}/VisusNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include")

endif()

if (VISUS_GUI)
	AddSwigLibrary(VisusGui ${CMAKE_CURRENT_SOURCE_DIR}/VisusGuiPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include")

endif()
  
if (VISUS_GUINODES)
	AddSwigLibrary(VisusGuiNodes ${CMAKE_CURRENT_SOURCE_DIR}/VisusGuiNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include")

endif()

if (VISUS_APPKIT)
	AddSwigLibrary(VisusAppKit ${CMAKE_CURRENT_SOURCE_DIR}/VisusAppKitPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../AppKit/include")
endif()




