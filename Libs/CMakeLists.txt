
add_subdirectory(Kernel)

if (VISUS_DB)
	add_subdirectory(Db)
endif()

if (VISUS_IDX)
	add_subdirectory(Idx)
endif()

if (VISUS_XIDX)
	add_subdirectory(XIdx)
endif()

if (VISUS_DATAFLOW)
	add_subdirectory(Dataflow)
endif()

if (VISUS_NODES)
	add_subdirectory(Nodes)
endif()

if (VISUS_GUI)
	add_subdirectory(Gui)
endif()
  
if (VISUS_GUINODES)
	add_subdirectory(GuiNodes)
endif()

if (VISUS_APPKIT)
	add_subdirectory(AppKit)
endif()

if (VISUS_PYTHON)
	add_subdirectory(swig)
endif()



