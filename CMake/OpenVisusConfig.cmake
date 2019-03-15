
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH )

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# todo: force when VISUS_PYTHON is not enabled
SET(VISUS_PYTHON "1" CACHE INTERNAL "")

# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusLibrary target_name)

	add_library(${target_name} SHARED IMPORTED GLOBAL)
	
	string(REPLACE "OpenVisus::" "" base_name ${target_name})
	
	# multiconfigurations
	if (CMAKE_CONFIGURATION_TYPES)
		
		if (EXISTS ${OpenVisus_DIR}/debug)
		  set(__debug_dir__ "debug/")
		else()
		  set(__debug_dir__ "")
		endif()

		if (WIN32)
		
			set_target_properties(${target_name} PROPERTIES
				IMPORTED_CONFIGURATIONS              "Debug"
				IMPORTED_CONFIGURATIONS              "Release"
				IMPORTED_CONFIGURATIONS              "RelWithDebInfo"
				IMPORTED_IMPLIB_DEBUG                "${OpenVisus_DIR}/${__debug_dir__}lib/Visus${base_name}.lib"
				IMPORTED_IMPLIB_RELEASE              "${OpenVisus_DIR}/lib/Visus${base_name}.lib"
				IMPORTED_IMPLIB_RELWITHDEBINFO       "${OpenVisus_DIR}/lib/Visus${base_name}.lib"
				INTERFACE_INCLUDE_DIRECTORIES        "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${base_name}")    	
				     
		elseif (APPLE)
		
			set_target_properties(${target_name} PROPERTIES
				IMPORTED_CONFIGURATIONS              "Debug"
				IMPORTED_CONFIGURATIONS              "Release"
				IMPORTED_CONFIGURATIONS              "RelWithDebInfo"
				IMPORTED_LOCATION_DEBUG              "${OpenVisus_DIR}/${__debug_dir__}bin/libVisus${base_name}.dylib"
				IMPORTED_LOCATION_RELEASE            "${OpenVisus_DIR}/bin/libVisus${base_name}.dylib"
				IMPORTED_LOCATION_RELWITHDEBINFO     "${OpenVisus_DIR}/bin/libVisus${base_name}.dylib"
				INTERFACE_INCLUDE_DIRECTORIES        "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${base_name}")    	
								
		endif()
		
	else()
			set_target_properties(${target_name} PROPERTIES 
				IMPORTED_LOCATION                    "${OpenVisus_DIR}/libVisus${base_name}.so"
				INTERFACE_INCLUDE_DIRECTORIES        "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${base_name}")  
	endif() 

endmacro()


if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	Message(STATUS "OpenVisus found in ${OpenVisus_DIR}")
	
	AddOpenVisusLibrary(OpenVisus::Kernel)
	AddOpenVisusLibrary(OpenVisus::Dataflow)
	AddOpenVisusLibrary(OpenVisus::Db)
	AddOpenVisusLibrary(OpenVisus::Idx)
	AddOpenVisusLibrary(OpenVisus::Nodes)
	
	set_target_properties(OpenVisus::Dataflow  PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Db        PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Idx       PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Db") 
	set_target_properties(OpenVisus::Nodes     PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Idx") 
	
	if (VISUS_PYTHON)
		set_target_properties(OpenVisus::Kernel PROPERTIES INTERFACE_COMPILE_DEFINITIONS VISUS_PYTHON=1)
	endif()
	
	if (EXISTS "${OpenVisus_DIR}/include/Gui")
	
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL  REQUIRED)
		
		if (WIN32)
			string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
		endif()
		
		AddOpenVisusLibrary(OpenVisus::Gui)
		AddOpenVisusLibrary(OpenVisus::GuiNodes)
		AddOpenVisusLibrary(OpenVisus::AppKit)
		
		set_target_properties(OpenVisus::Gui      PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
		set_target_properties(OpenVisus::GuiNodes PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow") 
		set_target_properties(OpenVisus::AppKit   PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes")
		
	endif()

endif()




