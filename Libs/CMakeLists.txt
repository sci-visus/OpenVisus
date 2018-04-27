
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

add_subdirectory(Swig)

# ///////////////////////////////////////////////////////////////////////////////
if (APPLE)

	macro(FixAnaconda Name)
	  set_target_properties(${Name} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
	endmacro()

    string(FIND "${PYTHON_EXECUTABLE}" "anaconda" is_anaconda)
	if ("${is_anaconda}" GREATER -1)

		FixAnaconda(Kernel)
		FixAnaconda(Dataflow)
		FixAnaconda(Db)
		FixAnaconda(Idx)
		FixAnaconda(Nodes)

		if (VISUS_GUI)
		  FixAnaconda(Gui)
		  FixAnaconda(GuiNodes)
		  FixAnaconda(AppKit)
		endif()	
	endif()
endif()







