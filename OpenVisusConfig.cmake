
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)

if(OpenVisus_FOUND)

	if (MSVC)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()
	
	get_filename_component(OpenVisus_ROOT "${OpenVisus_DIR}/../../../" REALPATH)
	MESSAGE(STATUS "OpenVisus_ROOT ${OpenVisus_ROOT} ")	

	# /////////////////////////////////////////////////////////////////
	macro(SetImportedLibLocation Name)
		string(TOLOWER "${CMAKE_CXX_COMPILER_ID}" __compiler_id__)
		if (MSVC)
			set_target_properties(OpenVisus::${Name}    PROPERTIES IMPORTED_IMPLIB   "${OpenVisus_ROOT}/lib/${Name}.lib")
		elseif (__compiler_id__ MATCHES ".*clang")
			set(CLANG 1)
			if(${CMAKE_VERSION} VERSION_LESS "3.20.0")
				set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_IMPLIB   "${OpenVisus_ROOT}/bin/lib${Name}.dylib")
			else()
				set_target_properties(OpenVisus::${Name}  PROPERTIES IMPORTED_LOCATION "${OpenVisus_ROOT}/bin/lib${Name}.dylib")
			endif()
		else()
			set_target_properties(OpenVisus::${Name}    PROPERTIES IMPORTED_IMPLIB   "${OpenVisus_ROOT}/bin/lib${Name}.so")
		endif()
	endmacro()	
	
	# ***** Visus ***** 
	add_library(OpenVisus::Visus SHARED IMPORTED GLOBAL)
	SetImportedLibLocation(Visus)
	set_target_properties(OpenVisus::Visus PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_ROOT}/include/Kernel;${OpenVisus_ROOT}/include/Db") 
	
	if (MINGW)
			#minimum support is Vista
			target_compile_options(OpenVisus::Visus PUBLIC -D_WIN32_WINNT=0x0600 -DWINVER=0x0600)
	endif()
	
	# ***** VisusGui ***** 
  if(EXISTS "${OpenVisus_ROOT}/QT_VERSION")
  
  	# by default GUI it'is disabled (don't want the user to have Qt5 installed)
		option(VISUS_GUI "Enable VISUS_GUI" OFF)
		
		if (VISUS_GUI)
			find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
			
			add_library(OpenVisus::VisusGui SHARED IMPORTED GLOBAL)
			SetImportedLibLocation(VisusGui)
			set_target_properties(OpenVisus::VisusGui PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${OpenVisus_ROOT}/include/Dataflow;${OpenVisus_ROOT}/include/Nodes;${OpenVisus_ROOT}/include/Gui")
			set_target_properties(OpenVisus::VisusGui PROPERTIES INTERFACE_LINK_LIBRARIES      "OpenVisus::Visus;Qt5::Core;Qt5::Widgets;Qt5::Gui;Qt5::OpenGL") 
		endif()
  endif()

endif()



