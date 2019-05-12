
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)

include(FindPackageHandleStandardArgs)
	
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusPythonLibrary)

	file(READ ${OpenVisus_DIR}/PYTHON_VERSION PYTHON_VERSION) 
	string(STRIP ${PYTHON_VERSION} PYTHON_VERSION)

	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)

	add_library(OpenVisus::Python SHARED IMPORTED GLOBAL)
	set_property(TARGET OpenVisus::Python APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	

	if (WIN32)
		list(LENGTH PYTHON_LIBRARY __n__)
		if (${__n__} EQUAL 1)
			set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_IMPLIB ${PYTHON_LIBRARY})
		else()
			list(FIND PYTHON_LIBRARY optimized __index__)
			math(EXPR __next_index__ "${__index__}+1")
			list(GET PYTHON_LIBRARY ${__next_index__} __release_library_location_)
			unset(PYTHON_LIBRARY CACHE)
			unset(PYTHON_LIBRARY)
			set(PYTHON_LIBRARY ${__release_library_location_})
			set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_IMPLIB ${__release_library_location_})
		endif()
	else()
		set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 
	endif()

endmacro()


# //////////////////////////////////////////////////////////////////////////////////
macro(AddOpenVisusLibrary Name Dependencies)

	string(REPLACE "OpenVisus::" "" base_name ${Name})

	if (WIN32)
		set(lib_release "${OpenVisus_DIR}/lib/Visus${base_name}.lib")
		set(lib_debug   "${OpenVisus_DIR}/debug/lib/Visus${base_name}.lib")
	elseif (APPLE)
		set(lib_release "${OpenVisus_DIR}/bin/libVisus${base_name}.dylib")
		set(lib_debug   "${OpenVisus_DIR}/debug/bin/libVisus${base_name}.dylib")	
	else()
		set(lib_release "${OpenVisus_DIR}/bin/libVisus${base_name}.so")
		set(lib_debug   "${OpenVisus_DIR}/debug/bin/libVisus${base_name}.so")	
	endif()

	if (EXISTS "${lib_release}")

		if (NOT EXISTS "${OpenVisus_DIR}/debug")
			set(lib_debug   "${lib_release}")
		endif()		

		add_library(${Name} SHARED IMPORTED GLOBAL)
		set_target_properties(${Name} PROPERTIES INTERFACE_LINK_LIBRARIES "${Dependencies}") 
		set_target_properties(${Name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_DIR}/include/Kernel;${OpenVisus_DIR}/include/${base_name}") 

		# multiconfigurations
		if (CMAKE_CONFIGURATION_TYPES)
	
			set_target_properties(${Name} PROPERTIES IMPORTED_CONFIGURATIONS "Debug")
			set_target_properties(${Name} PROPERTIES IMPORTED_CONFIGURATIONS "Release")
			set_target_properties(${Name} PROPERTIES IMPORTED_CONFIGURATIONS "RelWithDebInfo")
		
			# note: IMPORTED_<names> are different!
			if (WIN32)
	  			set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_DEBUG             "${lib_debug}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_RELEASE           "${lib_release}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_RELWITHDEBINFO    "${lib_release}")		
			else()
	  			set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_DEBUG           "${lib_debug}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_RELEASE         "${lib_release}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO  "${lib_release}")	
			endif()	
		
		else()
			if (WIN32)
				set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB                   "${lib_release}")
			else()
				set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION                 "${lib_release}")
			endif()
		endif()
	endif()

endmacro()


if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	message(STATUS "OpenVisus found in ${OpenVisus_DIR}")

	# python enabled/disabled
	if (EXISTS "${OpenVisus_DIR}/PYTHON_VERSION")
		SET(VISUS_PYTHON  "1" CACHE INTERNAL "VISUS_PYTHON")
	else()
		SET(VISUS_PYTHON  "0" CACHE INTERNAL "VISUS_PYTHON")
	endif()

	# gui enabled disabled
	if (EXISTS "${OpenVisus_DIR}/QT_VERSION")
		SET(VISUS_GUI  "1" CACHE INTERNAL "VISUS_PYTHON")
	else()
		SET(VISUS_GUI  "0" CACHE INTERNAL "VISUS_PYTHON")
	endif()
	
	if (VISUS_PYTHON)
		AddOpenVisusPythonLibrary()
		AddOpenVisusLibrary(OpenVisus::Kernel "OpenVisus::Python")
		set_target_properties(OpenVisus::Kernel PROPERTIES INTERFACE_COMPILE_DEFINITIONS VISUS_PYTHON=1)
	else()
		AddOpenVisusLibrary(OpenVisus::Kernel "")
		set_target_properties(OpenVisus::Kernel PROPERTIES INTERFACE_COMPILE_DEFINITIONS VISUS_PYTHON=0)
	endif()

	AddOpenVisusLibrary(OpenVisus::Dataflow "OpenVisus::Kernel")
	AddOpenVisusLibrary(OpenVisus::XIdx     "OpenVisus::Kernel")
	AddOpenVisusLibrary(OpenVisus::Db       "OpenVisus::Kernel")
	AddOpenVisusLibrary(OpenVisus::Idx      "OpenVisus::Db")
	AddOpenVisusLibrary(OpenVisus::Nodes    "OpenVisus::Idx")
	
	if (VISUS_GUI)
		find_package(Qt5 OPTIONAL_COMPONENTS Core Widgets Gui OpenGL QUIET)
		if (Qt5_FOUND)
			if (WIN32)
				string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
			endif()
			AddOpenVisusLibrary(OpenVisus::Gui       "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
			AddOpenVisusLibrary(OpenVisus::GuiNodes  "OpenVisus::Gui;OpenVisus::Dataflow")
			AddOpenVisusLibrary(OpenVisus::AppKit    "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes")
		else()
			message(STATUS "Qt5 not found, disabling it")
		endif()
	endif()

endif()




