CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

# qt-related
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

# qt-related
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD)
endif()

# swig related 
if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD)
endif()

# swig related
if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD) 
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  

# disable incremental linking for windows
SET(MSVC_INCREMENTAL_DEFAULT OFF)


PROJECT(OpenVisus) 

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# c++ standard
set(CMAKE_CXX_STANDARD              11)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)

if (APPLE)
	set(CMAKE_MACOSX_BUNDLE TRUE)
	set(CMAKE_MACOSX_RPATH  TRUE)
endif()

include(FindPackageHandleStandardArgs)

# see https://cmake.org/pipermail/cmake/2015-May/060700.html

if (CMAKE_CONFIGURATION_TYPES)
	set(ConfigName         $<CONFIG>)
	set(IntConfigName      ${CMAKE_CFG_INTDIR})
	set(InstallConfigName  \${CMAKE_INSTALL_CONFIG_NAME})
else()

	if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
		set(CMAKE_BUILD_TYPE "Release")
	endif()

	set(ConfigName         ${CMAKE_BUILD_TYPE})
	set(IntConfigName      ${CMAKE_BUILD_TYPE})
	set(InstallConfigName  ${CMAKE_BUILD_TYPE})

	MESSAGE(STATUS "CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
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

option(VISUS_NET    "Enable VISUS_NET"      ${VISUS_DEFAULT_NET})
option(VISUS_IMAGE  "Enable VISUS_IMAGE"    ${VISUS_DEFAULT_IMAGE})
option(VISUS_PYTHON "Enable VISUS_PYTHON"   ${VISUS_DEFAULT_PYTHON})
option(VISUS_GUI    "Enable VISUS_GUI"      ${VISUS_DEFAULT_GUI})
option(VISUS_GUI    "Enable VISUS_MODVISUS" ${VISUS_DEFAULT_MODVISUS})

message(STATUS "VISUS_NET       ${VISUS_NET}")
message(STATUS "VISUS_IMAGE     ${VISUS_IMAGE}")
message(STATUS "VISUS_PYTHON    ${VISUS_PYTHON}")
message(STATUS "VISUS_GUI       ${VISUS_GUI}")
message(STATUS "VISUS_MODVISUS  ${VISUS_MODVISUS}")


if (VISUS_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})	
endif()

if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
endif()

# //////////////////////////////////////////////////////////////
# see https://stackoverflow.com/questions/23327687/how-to-write-a-cmake-function-with-more-than-one-parameter-groups

include(CMakeParseArguments)

function(CopyToBinaryDirectory)

    cmake_parse_arguments(
        PARSED_ARGS 
        ""                             # boolean args
        "TARGET;DESTINATION;DIRECTORY" # 1-argument
        "FILES"                        # multiple args
        ${ARGN})

	if (PARSED_ARGS_FILES)
		foreach(__it__ ${PARSED_ARGS_FILES})
		
			set(__src__ ${__it__})
		
			if(NOT IS_ABSOLUTE ${__src__})
				set(__src__ ${CMAKE_CURRENT_SOURCE_DIR}/${__src__})
			endif()
		
			get_filename_component(__name__ ${__src__} NAME)
		
			add_custom_command(
				TARGET ${PARSED_ARGS_TARGET} POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E echo copy_if_different ${__src__} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}${__name__}
				COMMAND ${CMAKE_COMMAND} -E      copy_if_different ${__src__} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}${__name__})
		
		endforeach()
	endif()

	if (PARSED_ARGS_DIRECTORY)

		if(NOT IS_ABSOLUTE ${PARSED_ARGS_DIRECTORY})
			set(PARSED_ARGS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PARSED_ARGS_DIRECTORY})
		endif()

		add_custom_command(
			TARGET ${PARSED_ARGS_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E echo "copy_directory ${PARSED_ARGS_DIRECTORY} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}"
			COMMAND ${CMAKE_COMMAND} -E       copy_directory ${PARSED_ARGS_DIRECTORY} ${CMAKE_BINARY_DIR}/$<CONFIG>/${PROJECT_NAME}/${PARSED_ARGS_DESTINATION}
		)
		
	endif()

endfunction()

# //////////////////////////////////////////////////////////////
function(AssignSourceGroup)
	foreach(_it_ IN ITEMS ${ARGN})
		file(RELATIVE_PATH _group_ "${CMAKE_CURRENT_SOURCE_DIR}" "${_it_}")
		get_filename_component(_group_ "${_group_}" PATH)
		string(REPLACE "../"  ""   _group_  "${_group_}")
		string(REPLACE "../"  ""   _group_  "${_group_}")
		string(REPLACE "../"  ""   _group_  "${_group_}")
		string(REPLACE  "/"  "\\"  _group_  "${_group_}")
		source_group("${_group_}" FILES "${_it_}")
	endforeach()
endfunction()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# install-like step
if (1)

	if(VISUS_GUI)
		set(__qt_version_filename__ ${CMAKE_BINARY_DIR}/QT_VERSION)
		file(GENERATE OUTPUT ${__qt_version_filename__} CONTENT "${Qt5Core_VERSION_STRING}")
	endif()

	add_custom_target(CopyFiles ALL)
	set_property(TARGET CopyFiles PROPERTY FOLDER CMakePredefinedTargets)

	CopyToBinaryDirectory(TARGET CopyFiles FILES     LICENSE                    DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyFiles FILES     README.md                  DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyFiles FILES     datasets/visus.config      DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyFiles FILES     ${__qt_version_filename__} DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyFiles FILES     quick_tour.ipynb           DESTINATION ./)

	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Copyrights                 DESTINATION ./Copyrights)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Samples                    DESTINATION ./Samples)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY datasets                   DESTINATION ./datasets)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY scripts                    DESTINATION ./scripts)

	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Libs/Kernel/include/Visus   DESTINATION ./include/Kernel/Visus)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Libs/XIdx/include/Visus     DESTINATION ./include/XIdx/Visus)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Libs/Db/include/Visus       DESTINATION ./include/Db/Visus)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Libs/Dataflow/include/Visus DESTINATION ./include/Dataflow/Visus)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Libs/Nodes/include/Visus    DESTINATION ./include/Nodes/Visus)
	CopyToBinaryDirectory(TARGET CopyFiles DIRECTORY Libs/Gui/include/Visus      DESTINATION ./include/Gui/Visus)

	# copy needed windows *.dll
	if (WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${ConfigName}/${PROJECT_NAME}/bin")
		include(InstallRequiredSystemLibraries)
		CopyToBinaryDirectory(TARGET CopyFiles FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ./bin/)
	endif()
	
endif()

if (VISUS_PYTHON)

	macro(AddPythonTest Name)
		add_custom_target(${Name} ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/${ConfigName}/OpenVisus/.. ${Python_EXECUTABLE} -u ${ARGN})
		set_property(TARGET ${Name} PROPERTY FOLDER CMakePredefinedTargets)
	endmacro()

	AddPythonTest(test -m OpenVisus test)

	if (VISUS_GUI)
		AddPythonTest(UsePyQt5 -m OpenVisus use-pyqt5)
		AddPythonTest(viewer   -m OpenVisus viewer)
		AddPythonTest(viewer1  -m OpenVisus viewer1)
		AddPythonTest(viewer2  -m OpenVisus viewer2)
	endif()
endif()


