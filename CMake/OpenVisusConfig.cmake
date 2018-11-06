
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH )

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusPythonLibrary)
	add_library(OpenVisus::Python SHARED IMPORTED GLOBAL)
	file(READ "${OpenVisus_DIR}/PYTHON_VERSION" PYTHON_VERSION)
	message(STATUS "OpenVisus is using python ${PYTHON_VERSION}")
	# see https://github.com/Kitware/CMake/blob/master/Modules/FindPythonLibs.cmake
   if (EXISTS "${OpenVisus_DIR}/win32/python")
      set(PYTHON_EXECUTABLE "${OpenVisus_DIR}/win32/python/python.exe")
	endif()
	find_package(PythonLibs ${PYTHON_VERSION} REQUIRED)
	set_property(TARGET OpenVisus::Python APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	
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
        IMPORTED_IMPLIB_DEBUG           ${PYTHON_DEBUG_LIBRARY}
        IMPORTED_IMPLIB_RELEASE         ${PYTHON_RELEASE_LIBRARY}
        IMPORTED_IMPLIB_RELWITHDEBINFO  ${PYTHON_RELEASE_LIBRARY})  	
	else()
	  set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 
	endif()
endmacro()

# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusLibrary target_name)
	add_library(${target_name} SHARED IMPORTED GLOBAL)
	string(REPLACE "OpenVisus::" "" name ${target_name})
	set_property(TARGET ${target_name} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${name}")
	if (CMAKE_CONFIGURATION_TYPES)
		if (EXISTS ${OpenVisus_DIR}/debug)
		  set(__debug__ "debug/")
		else()
		  set(__debug__ "")
		endif()
		set_property(TARGET ${target_name} APPEND PROPERTY IMPORTED_CONFIGURATIONS Debug)
		set_property(TARGET ${target_name} APPEND PROPERTY IMPORTED_CONFIGURATIONS Release)
		set_property(TARGET ${target_name} APPEND PROPERTY IMPORTED_CONFIGURATIONS RelWithDebInfo)
		if (WIN32)
			set_target_properties(${target_name} PROPERTIES
			  IMPORTED_IMPLIB_DEBUG                ${OpenVisus_DIR}/${__debug__}lib/Visus${name}.lib
				IMPORTED_IMPLIB_RELEASE              ${OpenVisus_DIR}/lib/Visus${name}.lib
				IMPORTED_IMPLIB_RELWITHDEBINFO       ${OpenVisus_DIR}/lib/Visus${name}.lib)      
		elseif (APPLE)
			set_target_properties(${target_name} PROPERTIES
			  IMPORTED_LOCATION_DEBUG              ${OpenVisus_DIR}/${__debug__}bin/libVisus${name}.dylib
				IMPORTED_LOCATION_RELEASE            ${OpenVisus_DIR}/bin/libVisus${name}.dylib
				IMPORTED_LOCATION_RELWITHDEBINFO     ${OpenVisus_DIR}/bin/libVisus${name}.dylib)    					
		endif()
	else()
			set_target_properties(${target_name} PROPERTIES IMPORTED_LOCATION ${OpenVisus_DIR}/libVisus${name}.so)  
	endif() 
endmacro()


if(OpenVisus_FOUND)

	Message(STATUS "Visus found in ${OpenVisus_DIR}")
	
	AddOpenVisusPythonLibrary()
	
	AddOpenVisusLibrary(OpenVisus::Kernel)
	AddOpenVisusLibrary(OpenVisus::Dataflow)
	AddOpenVisusLibrary(OpenVisus::Db)
	AddOpenVisusLibrary(OpenVisus::Idx)
	AddOpenVisusLibrary(OpenVisus::Nodes)
	
	set_property(TARGET OpenVisus::Kernel    APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Python") 
	set_property(TARGET OpenVisus::Dataflow  APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_property(TARGET OpenVisus::Db        APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 
	set_property(TARGET OpenVisus::Idx       APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Db") 
	set_property(TARGET OpenVisus::Nodes     APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Idx") 
	
	if (EXISTS "${OpenVisus_DIR}/include/Gui")
	
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL  REQUIRED)
		
		AddOpenVisusLibrary(OpenVisus::Gui)
		AddOpenVisusLibrary(OpenVisus::GuiNodes)
		AddOpenVisusLibrary(OpenVisus::AppKit)
		
		set_property(TARGET OpenVisus::Gui      APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
		set_property(TARGET OpenVisus::GuiNodes APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow") 
		set_property(TARGET OpenVisus::AppKit   APPEND PROPERTY INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes")
		
	endif()

endif()




