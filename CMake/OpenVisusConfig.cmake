
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH )

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusLibrary name deps)

	if (EXISTS "${OpenVisus_DIR}/include/${name}")
	
		add_library(OpenVisus::${name} SHARED IMPORTED GLOBAL)
	   
	   set_target_properties(OpenVisus::${name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${name}")
	   	
	   set_property(TARGET OpenVisus::${name} APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)
	   set_property(TARGET OpenVisus::${name} APPEND PROPERTY IMPORTED_CONFIGURATIONS Release)
	   set_property(TARGET OpenVisus::${name} APPEND PROPERTY IMPORTED_CONFIGURATIONS RelWithDebInfo)
	   
	   if (WIN32)
	   
	   	if (EXISTS ${OpenVisus_DIR}/debug)
			   set_target_properties(OpenVisus::${name} PROPERTIES
					IMPORTED_IMPLIB_DEBUG               ${OpenVisus_DIR}/debug/lib/Visus${name}.lib
					IMPORTED_LOCATION_DEBUG             ${OpenVisus_DIR}/debug/Visus${name}.dll)
			else()
			   set_target_properties(OpenVisus::${name} PROPERTIES
					IMPORTED_IMPLIB_DEBUG               ${OpenVisus_DIR}/lib/Visus${name}.lib
					IMPORTED_LOCATION_DEBUG             ${OpenVisus_DIR}/Visus${name}.dll)
					
			endif()
				
		   set_target_properties(OpenVisus::${name} PROPERTIES
				IMPORTED_IMPLIB_RELEASE              ${OpenVisus_DIR}/lib/Visus${name}.lib
				IMPORTED_LOCATION_RELEASE            ${OpenVisus_DIR}/bin/Visus${name}.dll)      
				
		   
		   set_target_properties(OpenVisus::${name} PROPERTIES
				IMPORTED_IMPLIB_RELWITHDEBINFO       ${OpenVisus_DIR}/lib/Visus${name}.lib
				IMPORTED_LOCATION_RELWITHDEBINFO     ${OpenVisus_DIR}/bin/Visus${name}.dll)    				
				
		endif()  
		
		set_property(TARGET OpenVisus::${name} APPEND PROPERTY  INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/${name}")
		
		foreach(dep ${deps})
			set_property(TARGET OpenVisus::${name} APPEND PROPERTY  INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/${dep}")
			set_property(TARGET OpenVisus::${name} APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "OpenVisus::${dep}")
		endforeach()
		   
	endif()

endmacro()

if(OpenVisus_FOUND)

   Message("Visus found in ${OpenVisus_DIR}")
   
   AddOpenVisusLibrary(Kernel   "")
   AddOpenVisusLibrary(Dataflow "Kernel")
   AddOpenVisusLibrary(Db       "Kernel")
   AddOpenVisusLibrary(Idx      "Kernel;Db")
   AddOpenVisusLibrary(Nodes    "Kernel;Dataflow;Db;Idx")

   AddOpenVisusLibrary(Gui      "Kernel")
   AddOpenVisusLibrary(GuiNodes "Kernel;Gui;Dataflow")
   AddOpenVisusLibrary(AppKit   "Kernel;Gui;Dataflow;Db;Idx;Nodes;GuiNodes")

	if (TARGET OpenVisus::Gui)
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL  REQUIRED)
		set_property(TARGET OpenVisus::Gui APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
	endif()

endif()




