
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH )

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

if(OpenVisus_FOUND)
   Message("Visus found in ${OpenVisus_DIR}")
   
   foreach(it Kernel Dataflow Db Idx Nodes Gui GuiNodes AppKit)
   
	   add_library(Visus${it} SHARED IMPORTED GLOBAL)
	   
	   set_target_properties(Visus${it} PROPERTIES   
	   	INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${it}")
	   
	   set_property(TARGET Visus${it} APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)
	   
	   set_target_properties(Visus${it} PROPERTIES
			IMPORTED_IMPLIB_DEBUG               ${OpenVisus_DIR}/debug/lib/Visus${it}.lib
			IMPORTED_LOCATION_DEBUG             ${OpenVisus_DIR}/debug/Visus${it}.dll)
	   
	   set_property(TARGET Visus${it} APPEND PROPERTY IMPORTED_CONFIGURATIONS Release)
	   
	   set_target_properties(Visus${it} PROPERTIES
			IMPORTED_IMPLIB_RELEASE              ${OpenVisus_DIR}/lib/Visus${it}.lib
			IMPORTED_LOCATION_RELEASE            ${OpenVisus_DIR}/Visus${it}.dll)      
			
	   set_property(TARGET Visus${it} APPEND PROPERTY IMPORTED_CONFIGURATIONS RelWithDebInfo)
	   
	   set_target_properties(Visus${it} PROPERTIES
			IMPORTED_IMPLIB_RELWITHDEBINFO       ${OpenVisus_DIR}/lib/Visus${it}.lib
			IMPORTED_LOCATION_RELWITHDEBINFO     ${OpenVisus_DIR}/Visus${it}.dll)     			
			
   endforeach()
	
   set_property(TARGET VisusDataflow   APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusKernel")
   set_property(TARGET VisusDb         APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusKernel")
   set_property(TARGET VisusIdx        APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusDb")
   set_property(TARGET VisusNodes      APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusIdx;VisusDataflow")
   set_property(TARGET VisusGui        APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusKernel")
   set_property(TARGET VisusGuiNodes   APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusGui;VisusDataflow")
   set_property(TARGET VisusAppKit     APPEND PROPERTY  INTERFACE_LINK_LIBRARIES "VisusNodes;VisusGuiNodes")
   
   # todo better
   set_property(TARGET VisusIdx        APPEND PROPERTY  INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Db")
   set_property(TARGET VisusNodes      APPEND PROPERTY  INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Dataflow;${OpenVisus_DIR}/include/Db;${OpenVisus_DIR}/include/Idx")
   set_property(TARGET VisusGuiNodes   APPEND PROPERTY  INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Gui;${OpenVisus_DIR}/include/Dataflow")
   set_property(TARGET VisusAppKit     APPEND PROPERTY  INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Gui;${OpenVisus_DIR}/include/Dataflow;${OpenVisus_DIR}/include/Db;;${OpenVisus_DIR}/include/Idx;;${OpenVisus_DIR}/include/Nodes;;${OpenVisus_DIR}/include/GuiNodes")


endif()




