
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# //////////////////////////////////////////////////////////////////////////////////
macro(AddPythonLibrary Name)

	file(READ ${OpenVisus_DIR}/PYTHON_VERSION PYTHON_VERSION) 
	string(STRIP ${PYTHON_VERSION} PYTHON_VERSION)

	message(STATUS "AddPythonLibrary PYTHON_VERSION=${PYTHON_VERSION}")

	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	message(STATUS "AddPythonLibrary PYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}")
	
	find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)
	message(STATUS "AddPythonLibrary PYTHON_INCLUDE_DIRS=${PYTHON_INCLUDE_DIRS}")
	message(STATUS "AddPythonLibrary PYTHON_LIBRARIES=${PYTHON_LIBRARIES}")
	
	add_library(${Name} SHARED IMPORTED GLOBAL)
	
	if (WIN32)
		list(LENGTH PYTHON_LIBRARY __n__)
		# example debug;aaaa;optimized;bbb
		if (${__n__} GREATER 1)
			list(FIND PYTHON_LIBRARY optimized __index__)
			if (${__index__} EQUAL -1)
				MESSAGE(ERROR "Problem with find python")
			endif()
			math(EXPR __next_index__ "${__index__}+1")
			list(GET PYTHON_LIBRARY ${__next_index__} PYTHON_LIBRARY)
		endif()
	endif()

	set_property(TARGET ${Name} APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	
	set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 

endmacro()


# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusLibrary Name)

	add_library(${Name} SHARED IMPORTED GLOBAL)
	
	string(REPLACE "OpenVisus::" "" base_name ${Name})

	if (WIN32)
		set(lib_ext ".lib")
	elseif (APPLE)
		set(lib_ext ".dylib")
	else()
		set(lib_ext ".so")
	endif()

	set_target_properties(${Name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${base_name}") 
	
	# multiconfigurations
	if (CMAKE_CONFIGURATION_TYPES)

		set_target_properties(${Name}    PROPERTIES IMPORTED_CONFIGURATIONS "Debug")
		set_target_properties(${Name}    PROPERTIES IMPORTED_CONFIGURATIONS "Release")
		set_target_properties(${Name}    PROPERTIES IMPORTED_CONFIGURATIONS "RelWithDebInfo")

		if (EXISTS "${OpenVisus_DIR}/debug")
			set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_DEBUG          "${OpenVisus_DIR}/debug/bin/libVisus${base_name}${lib_ext}")
		else()
			set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_DEBUG          "${OpenVisus_DIR}/bin/libVisus${base_name}${lib_ext}")
		endif()

		set_target_properties(${Name}    PROPERTIES IMPORTED_LOCATION_RELEASE        "${OpenVisus_DIR}/bin/libVisus${base_name}${lib_ext}")
		set_target_properties(${Name}    PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO "${OpenVisus_DIR}/bin/libVisus${base_name}${lib_ext}")    	
		
	else()
			set_target_properties(${Name} PROPERTIES  IMPORTED_LOCATION               "${OpenVisus_DIR}/bin/libVisus${base_name}${lib_ext}")   
	endif() 

endmacro()


if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	message(STATUS "OpenVisus found in ${OpenVisus_DIR}")

	AddPythonLibrary(OpenVisus::Python)
	
	AddOpenVisusLibrary(OpenVisus::Kernel)
	set_target_properties(OpenVisus::Kernel PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Python") 

	AddOpenVisusLibrary(OpenVisus::Dataflow)
	set_target_properties(OpenVisus::Dataflow PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 

	AddOpenVisusLibrary(OpenVisus::Db)
	set_target_properties(OpenVisus::Db       PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel") 

	AddOpenVisusLibrary(OpenVisus::Idx)
	set_target_properties(OpenVisus::Idx      PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Db") 

	AddOpenVisusLibrary(OpenVisus::Nodes)
	set_target_properties(OpenVisus::Nodes    PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Idx") 
	
	if (EXISTS "${OpenVisus_DIR}/QT_VERSION")
	
		find_package(Qt5 OPTIONAL_COMPONENTS Core Widgets Gui OpenGL QUIET)
		
		if (Qt5_FOUND)
		
			if (WIN32)
				string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
			endif()
		
			AddOpenVisusLibrary(OpenVisus::Gui)
			set_target_properties(OpenVisus::Gui      PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL") 

			AddOpenVisusLibrary(OpenVisus::GuiNodes)
			set_target_properties(OpenVisus::GuiNodes PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow") 

			AddOpenVisusLibrary(OpenVisus::AppKit)
			set_target_properties(OpenVisus::AppKit   PROPERTIES INTERFACE_LINK_LIBRARIES "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes") 
		
		else()
			message(STATUS "Qt5 not found, disabling it")
		endif()
	endif()

endif()




