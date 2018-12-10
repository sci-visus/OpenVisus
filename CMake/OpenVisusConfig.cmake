
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH )

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# //////////////////////////////////////////////////////////////////////////////////
macro(OpenVisusFindPythonLibrary)

	add_library(OpenVisus::Python SHARED IMPORTED GLOBAL)
	
	file(READ "${OpenVisus_DIR}/PYTHON_VERSION" PYTHON_VERSION)
	message(STATUS "OpenVisus is using python ${PYTHON_VERSION}")
	
	# see https://github.com/Kitware/CMake/blob/master/Modules/FindPythonLibs.cmake
   if (EXISTS "${OpenVisus_DIR}/win32/python")
      set(PYTHON_EXECUTABLE "${OpenVisus_DIR}/win32/python/python.exe")
      message(STATUS "OpenVisus is using embedded python ${PYTHON_EXECUTABLE}")
	endif()
	
	find_package(PythonLibs ${PYTHON_VERSION} REQUIRED)
	MESSAGE(STATUS "OpenVisus found PYTHON_INCLUDE_DIRS ${PYTHON_INCLUDE_DIRS}")
	
	set_target_properties(OpenVisus::Python PROPERTIES 
		INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	
	
	if (WIN32)
	
  	   list(LENGTH PYTHON_LIBRARY __n__)
    	if (${__n__} EQUAL 1)

    		set(PYTHON_DEBUG_LIBRARY     ${PYTHON_LIBRARY})
    		set(PYTHON_RELEASE_LIBRARY   ${PYTHON_LIBRARY})

    	else()

      	list(FIND PYTHON_LIBRARY optimized __index__)
      	if (${__index__} EQUAL -1)
      		MESSAGE(ERROR "Problem with find python")
      	endif()
      	math(EXPR __next_index__ "${__index__}+1")
      	list(GET PYTHON_LIBRARY ${__next_index__} PYTHON_RELEASE_LIBRARY)
      	list(FIND PYTHON_LIBRARY debug __index__)
      	if (${__index__} EQUAL -1)
      		MESSAGE(ERROR "Problem with find python")
      	endif()
      	math(EXPR __next_index__ "${__index__}+1")
      	list(GET PYTHON_LIBRARY ${__next_index__} PYTHON_DEBUG_LIBRARY)
    	endif()

    	set_target_properties(OpenVisus::Python PROPERTIES
        IMPORTED_IMPLIB_DEBUG           "${PYTHON_DEBUG_LIBRARY}"
        IMPORTED_IMPLIB_RELEASE         "${PYTHON_RELEASE_LIBRARY}"
        IMPORTED_IMPLIB_RELWITHDEBINFO  "${PYTHON_RELEASE_LIBRARY}")
        
	  MESSAGE(STATUS "OpenVisus found PYTHON_DEBUG_LIBRARY   ${PYTHON_DEBUG_LIBRARY}")  	
	  MESSAGE(STATUS "OpenVisus found PYTHON_RELEASE_LIBRARY ${PYTHON_RELEASE_LIBRARY}") 
        
	else()
	
	  set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_LOCATION "${PYTHON_LIBRARY}") 
	  MESSAGE(STATUS "OpenVisus found PYTHON_LIBRARY ${PYTHON_LIBRARY}")
	
	endif()
	
endmacro()

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
	
	OpenVisusFindPythonLibrary()
	
	AddOpenVisusLibrary(OpenVisus::Kernel)
	AddOpenVisusLibrary(OpenVisus::Dataflow)
	AddOpenVisusLibrary(OpenVisus::Db)
	AddOpenVisusLibrary(OpenVisus::Idx)
	AddOpenVisusLibrary(OpenVisus::Nodes)
	
	set_target_properties(OpenVisus::Kernel    PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Python") 
	set_target_properties(OpenVisus::Dataflow  PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Db        PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_target_properties(OpenVisus::Idx       PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Db") 
	set_target_properties(OpenVisus::Nodes     PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Idx") 
	
	if (EXISTS "${OpenVisus_DIR}/include/Gui")
	
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL  REQUIRED)
		
		AddOpenVisusLibrary(OpenVisus::Gui)
		AddOpenVisusLibrary(OpenVisus::GuiNodes)
		AddOpenVisusLibrary(OpenVisus::AppKit)
		
		set_target_properties(OpenVisus::Gui      PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
		set_target_properties(OpenVisus::GuiNodes PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow") 
		set_target_properties(OpenVisus::AppKit   PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes")
		
	endif()
	


endif()




