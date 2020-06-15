
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

if(OpenVisus_FOUND)

	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	get_filename_component(OpenVisus_ROOT "${OpenVisus_DIR}/../../../" REALPATH)
	MESSAGE(STATUS "OpenVisus_ROOT ${OpenVisus_ROOT} ")

	macro(AddOpenVisusLibrary Name)
		add_library(OpenVisus::${Name} SHARED IMPORTED GLOBAL)
		set_target_properties(OpenVisus::${Name}      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_ROOT}/include/${Name}") 
		if (WIN32)
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_ROOT}/lib/Visus${Name}.lib")
		elseif (APPLE)
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_ROOT}/bin/libVisus${Name}.dylib")
		else()
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_ROOT}/bin/libVisus${Name}.so")
		endif()
		if (NOT "${ARGN}"  STREQUAL "" )
			set_target_properties(OpenVisus::${Name}  PROPERTIES INTERFACE_LINK_LIBRARIES "${ARGN}") 
		endif()
	endmacro()
	
	AddOpenVisusLibrary(Kernel)
	AddOpenVisusLibrary(XIdx     "OpenVisus::Kernel")
	AddOpenVisusLibrary(Db       "OpenVisus::Kernel")
	AddOpenVisusLibrary(Dataflow "OpenVisus::Kernel")
	AddOpenVisusLibrary(Nodes    "OpenVisus::Db;OpenVisus::Dataflow")
	
  if(EXISTS "${OpenVisus_ROOT}/QT_VERSION")

	# by default GUI it'is disabled (don't want the user to have Qt5 installed)
	option(VISUS_GUI "Enable VISUS_GUI" OFF)

	if (VISUS_GUI)
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
		AddOpenVisusLibrary(Gui "OpenVisus::Nodes;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL")
	endif()
  endif()

endif()



