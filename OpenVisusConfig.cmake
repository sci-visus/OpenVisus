
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	macro(AddOpenVisusLibrary Name)
		add_library(OpenVisus::${Name} SHARED IMPORTED GLOBAL)
		set_target_properties(OpenVisus::${Name}    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/${Name}") 
		if (WIN32)
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_DIR}/lib/Visus${Name}.lib")
		elseif (APPLE)
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_DIR}/bin/libVisus${Name}.dylib")
		else()
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_DIR}/bin/libVisus${Name}.so")
		endif()
		set_target_properties(OpenVisus::${Name}    PROPERTIES INTERFACE_LINK_LIBRARIES ${ARGN}) 
	endmacro()
	
	AddOpenVisusLibrary(Kernel   "")
	AddOpenVisusLibrary(XIdx     "OpenVisus::Kernel")
	AddOpenVisusLibrary(Db       "OpenVisus::Kernel")
	AddOpenVisusLibrary(Dataflow "OpenVisus::Kernel")
	AddOpenVisusLibrary(Nodes    "OpenVisus::Db;OpenVisus::Dataflow")
	
  if(EXISTS "${OpenVisus_DIR}/QT_VERSION")
   	option(VISUS_GUI default 1)
    find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
    AddOpenVisusLibrary(Gui "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
  endif()

endif()



