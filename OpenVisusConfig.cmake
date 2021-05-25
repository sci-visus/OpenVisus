
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

string( TOLOWER "${CMAKE_CXX_COMPILER_ID}" COMPILER_ID )
MESSAGE(STATUS "COMPILER_ID ${COMPILER_ID}")

# detect compiler
if (MSVC)
	MESSAGE(STATUS "Detected visual studio (C++ __MSVC_VER - WIN32)")
elseif (COMPILER_ID MATCHES ".*clang")
	set(CLANG 1)
	MESSAGE(STATUS "Detected apple clang (C++: __clang__ __APPLE__)")
else()
	set(GNUC  1)
	if (MINGW)
		MESSAGE(STATUS "Detected GNU on windows (C++ __GNUC__ WIN32)")
		#minimum support is Vista
		add_compile_options(-D_WIN32_WINNT=0x0600)
		add_compile_options(-DWINVER=0x0600)
		set(WIN32 1)
	elseif (APPLE)
		MESSAGE(STATUS "Detected GNU on apple (C++ __GNUC__ __APPLE__)")
		set(APPLE 1)
	else()
		MESSAGE(STATUS "Detected GNU on unix (C++ __GNUC__)")
	endif()
endif()

if(OpenVisus_FOUND)

	if (MSVC)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()

	get_filename_component(OpenVisus_ROOT "${OpenVisus_DIR}/../../../" REALPATH)
	MESSAGE(STATUS "OpenVisus_ROOT ${OpenVisus_ROOT} ")

	macro(AddOpenVisusLibrary Name)
		add_library(OpenVisus::${Name} SHARED IMPORTED GLOBAL)
		set_target_properties(OpenVisus::${Name}      PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_ROOT}/include/${Name}") 
		if (MSVC)
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_ROOT}/lib/Visus${Name}.lib")
		elseif (CLANG)
		
			# not sure what we need here
			if(${CMAKE_VERSION} VERSION_LESS "3.20.0")
				set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB   "${OpenVisus_ROOT}/bin/libVisus${Name}.dylib")
			else()
				set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_LOCATION "${OpenVisus_ROOT}/bin/libVisus${Name}.dylib")
			endif()
		
		else()
			set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB "${OpenVisus_ROOT}/bin/libVisus${Name}.so")
		endif()
		if (NOT "${ARGN}"  STREQUAL "" )
			set_target_properties(OpenVisus::${Name}  PROPERTIES INTERFACE_LINK_LIBRARIES "${ARGN}") 
		endif()
	endmacro()
	
	AddOpenVisusLibrary(Kernel)
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



