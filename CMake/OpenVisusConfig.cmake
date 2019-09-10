

# //////////////////////////////////////////////////////////////////////////////////
macro(AddImportedOpenVisusLibrary OpenVisus_DIR Name Dependencies)

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
			set_target_properties(${Name} PROPERTIES IMPORTED_CONFIGURATIONS "MinSizeRel")
		
			# note: IMPORTED_<names> are different!
			if (WIN32)
	  			set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_DEBUG             "${lib_debug}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_RELEASE           "${lib_release}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_RELWITHDEBINFO    "${lib_release}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_IMPLIB_MINSIZEREL        "${lib_release}")
			else()
	  			set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_DEBUG           "${lib_debug}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_RELEASE         "${lib_release}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_RELWITHDEBINFO  "${lib_release}")
		  		set_target_properties(${Name} PROPERTIES IMPORTED_LOCATION_MINSIZEREL      "${lib_release}")	
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

# ///////////////////////////////////////////////////////////////
macro(AddOpenVisusPythonLibraries OpenVisus_DIR)

	if (EXISTS "${OpenVisus_DIR}/PYTHON_VERSION")

		SET(VISUS_PYTHON  "1" CACHE INTERNAL "VISUS_PYTHON")

		# force the version to be the same
		file(READ ${OpenVisus_DIR}/PYTHON_VERSION PYTHON_VERSION) 
		string(STRIP ${PYTHON_VERSION} PYTHON_VERSION)

		FindPythonLibrary()
		AddImportedOpenVisusLibrary(${OpenVisus_DIR} OpenVisus::Kernel "OpenVisus::Python")
		set_target_properties(OpenVisus::Kernel PROPERTIES INTERFACE_COMPILE_DEFINITIONS VISUS_PYTHON=1)
	else()
		ForceUnset(VISUS_PYTHON)
		AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::Kernel "")
	endif()

	AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::Dataflow "OpenVisus::Kernel")
	AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::XIdx     "OpenVisus::Kernel")
	AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::Db       "OpenVisus::Kernel")
	AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::Idx      "OpenVisus::Db")
	AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::Nodes    "OpenVisus::Idx")
	
	if (EXISTS "${OpenVisus_DIR}/QT_VERSION")
		SET(VISUS_GUI  "1" CACHE INTERNAL "VISUS_PYTHON")
		find_package(Qt5 OPTIONAL_COMPONENTS Core Widgets Gui OpenGL QUIET)
		if (Qt5_FOUND)
			if (WIN32)
				string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
			endif()
			AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::Gui       "OpenVisus::Kernel;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
			AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::GuiNodes  "OpenVisus::Gui;OpenVisus::Dataflow")
			AddImportedOpenVisusLibrary(${OpenVisus_DIR}  OpenVisus::AppKit    "OpenVisus::Gui;OpenVisus::Dataflow;OpenVisus::Nodes;OpenVisus::GuiNodes")
		else()
			message(STATUS "Qt5 not found, disabling it")
		endif()
	else()
		ForceUnset(VISUS_GUI)
	endif()

endmacro()

find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)
if(OpenVisus_FOUND)
	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()
	message(STATUS "OpenVisus found in ${OpenVisus_DIR}")
	include(${OpenVisus_DIR}/CMake/VisusMacros.cmake)
	AddOpenVisusPythonLibraries(${OpenVisus_DIR})
endif()




