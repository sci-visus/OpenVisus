CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # qt-related
endif()

if (POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD) # qt-related
endif()

if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD) # swig related 
endif()

if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD)  # swig related
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  
SET(MSVC_INCREMENTAL_DEFAULT OFF) # disable incremental linking for windows

PROJECT(OpenVisus) 

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD              11)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)

if (APPLE)
	set(CMAKE_MACOSX_BUNDLE TRUE)
	set(CMAKE_MACOSX_RPATH  TRUE)
endif()

include(FindPackageHandleStandardArgs)

if (CMAKE_CONFIGURATION_TYPES)
	set(ConfigName         $<CONFIG>)
	set(IntConfigName      ${CMAKE_CFG_INTDIR})
else()
	if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
		set(CMAKE_BUILD_TYPE "Release")
	endif()
	set(ConfigName         ${CMAKE_BUILD_TYPE})
	set(IntConfigName      ${CMAKE_BUILD_TYPE})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/lib)

set (CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE STRING "CMAKE_INSTALL_PREFIX" FORCE)

set(VISUS_DEFAULT_NET      ON)
set(VISUS_DEFAULT_IMAGE    ON)
set(VISUS_DEFAULT_PYTHON   ON)
set(VISUS_DEFAULT_GUI      ON)
set(VISUS_DEFAULT_MODVISUS ON)

if (WIN32 OR APPLE)
	set(VISUS_DEFAULT_MODVISUS OFF)
endif()

option(VISUS_NET      "Enable VISUS_NET"      ${VISUS_DEFAULT_NET})
option(VISUS_IMAGE    "Enable VISUS_IMAGE"    ${VISUS_DEFAULT_IMAGE})
option(VISUS_PYTHON   "Enable VISUS_PYTHON"   ${VISUS_DEFAULT_PYTHON})
option(VISUS_GUI      "Enable VISUS_GUI"      ${VISUS_DEFAULT_GUI})
option(VISUS_MODVISUS "Enable VISUS_MODVISUS" ${VISUS_DEFAULT_MODVISUS})

if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
endif()

if (VISUS_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
	macro(LinkPythonIfNeeded Name)
		if (WIN32)
			# for windows most of the extensions do not work in debug, so I'm always using python release version
			target_include_directories(${Name} PUBLIC ${Python_INCLUDE_DIRS})
			target_link_libraries(${Name}      PUBLIC ${Python_LIBRARY_RELEASE})
		else()
			target_link_libraries(${Name} PUBLIC Python::Module)
		endif()
		target_compile_definitions(${Name} PUBLIC VISUS_PYTHON=1)
	endmacro()
else()
	macro(LinkPythonIfNeeded Name)
		# nothing to do
	endmacro()
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# see https://stackoverflow.com/questions/23327687/how-to-write-a-cmake-function-with-more-than-one-parameter-groups
include(CMakeParseArguments)
function(CopyToBinaryDirectory)
	cmake_parse_arguments(
		PARSED_ARGS 
		""                      # boolean args
		"DESTINATION;DIRECTORY" # 1-argument
		"FILES"                 # multiple args
		${ARGN})
	if (PARSED_ARGS_FILES)
		foreach(__it__ ${PARSED_ARGS_FILES})
			set(__src__ ${__it__})
			if(NOT IS_ABSOLUTE ${__src__})
				set(__src__ ${CMAKE_CURRENT_SOURCE_DIR}/${__src__})
			endif()
			get_filename_component(__name__ ${__src__} NAME)
			add_custom_command(
				TARGET CopyFiles POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E echo copy_if_different ${__src__} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}${__name__}
				COMMAND ${CMAKE_COMMAND} -E      copy_if_different ${__src__} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}${__name__})
		endforeach()
	endif()
	if (PARSED_ARGS_DIRECTORY)
		if(NOT IS_ABSOLUTE ${PARSED_ARGS_DIRECTORY})
			set(PARSED_ARGS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PARSED_ARGS_DIRECTORY})
		endif()
		add_custom_command(
			TARGET CopyFiles POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E echo "copy_directory ${PARSED_ARGS_DIRECTORY} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}"
			COMMAND ${CMAKE_COMMAND} -E       copy_directory ${PARSED_ARGS_DIRECTORY} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION})
	endif()
endfunction()

# copy files to binary directory
if (1)
	add_custom_target(CopyFiles ALL)
	set_property(TARGET CopyFiles PROPERTY FOLDER CMakePredefinedTargets)

	if(VISUS_GUI)
		set(__qt_version_filename__ ${CMAKE_BINARY_DIR}/QT_VERSION)
		file(GENERATE OUTPUT ${__qt_version_filename__} CONTENT "${Qt5Core_VERSION_STRING}")
	endif()

	FILE(GLOB PyFiles Libs/swig/*.py)

	CopyToBinaryDirectory(FILES     LICENSE                     DESTINATION ./)
	CopyToBinaryDirectory(FILES     README.md                   DESTINATION ./)
	CopyToBinaryDirectory(FILES     datasets/visus.config       DESTINATION ./)
	CopyToBinaryDirectory(FILES     quick_tour.ipynb            DESTINATION ./)
	CopyToBinaryDirectory(FILES     ${__qt_version_filename__}  DESTINATION ./)
	CopyToBinaryDirectory(FILES     ${PyFiles}                  DESTINATION ./)

	CopyToBinaryDirectory(DIRECTORY Copyrights                  DESTINATION ./Copyrights)
	CopyToBinaryDirectory(DIRECTORY Samples                     DESTINATION ./Samples)
	CopyToBinaryDirectory(DIRECTORY datasets                    DESTINATION ./datasets)

	CopyToBinaryDirectory(DIRECTORY Libs/Kernel/include/Visus   DESTINATION ./include/Kernel/Visus)
	CopyToBinaryDirectory(DIRECTORY Libs/XIdx/include/Visus     DESTINATION ./include/XIdx/Visus)
	CopyToBinaryDirectory(DIRECTORY Libs/Db/include/Visus       DESTINATION ./include/Db/Visus)
	CopyToBinaryDirectory(DIRECTORY Libs/Dataflow/include/Visus DESTINATION ./include/Dataflow/Visus)
	CopyToBinaryDirectory(DIRECTORY Libs/Nodes/include/Visus    DESTINATION ./include/Nodes/Visus)
	CopyToBinaryDirectory(DIRECTORY Libs/Gui/include/Visus      DESTINATION ./include/Gui/Visus)
	
	if (WIN32)
		# copy needed windows *.dll
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${ConfigName}/${PROJECT_NAME}/bin")
		include(InstallRequiredSystemLibraries)
		CopyToBinaryDirectory(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ./bin/)
	endif()
	
endif()



