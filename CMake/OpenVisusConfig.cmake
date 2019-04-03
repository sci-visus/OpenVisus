
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusLibrary target_name link_libraries)

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
	
	set_target_properties(${target_name} PROPERTIES INTERFACE_LINK_LIBRARIES "${link_libraries}") 

endmacro()


if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	message(STATUS "OpenVisus found in ${OpenVisus_DIR}")
	
	AddOpenVisusLibrary(OpenVisus::Kernel   "")
	AddOpenVisusLibrary(OpenVisus::Dataflow "OpenVisus::Kernel")
	AddOpenVisusLibrary(OpenVisus::Db       "OpenVisus::Kernel")
	AddOpenVisusLibrary(OpenVisus::Idx      "OpenVisus::Db")
	AddOpenVisusLibrary(OpenVisus::Nodes    "OpenVisus::Idx")
	
	if (EXISTS "${OpenVisus_DIR}/include/Gui")
	
		find_package(Qt5 OPTIONAL_COMPONENTS Core Widgets Gui OpenGL QUIET)
		
		if (Qt5_FOUND)
		
			if (WIN32)
				string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
			endif()
		
			AddOpenVisusLibrary(OpenVisus::Gui      "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
			AddOpenVisusLibrary(OpenVisus::GuiNodes "OpenVisus::Gui;OpenVisus::Dataflow")
			AddOpenVisusLibrary(OpenVisus::AppKit   "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes")
		
		else()
			message(STATUS "Qt5 not found, disabling it")
		endif()
	endif()

endif()




