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

# //////////////////////////////////////////////////////////
macro(AddSwigLibrary NamePy WrappedLib SwigFile)

	if (VISUS_PYTHON)

		# this is for generated C++ and header files
		set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${IntConfigName}) 

		set(CMAKE_SWIG_OUTDIR ${SWIG_OUTFILE_DIR}/OpenVisus) # this is for *.py generated files
		set (UseSWIG_TARGET_NAME_PREFERENCE STANDARD)

		set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-threads;-extranative")

		# important for good name generation id Debug mode (https://gitlab.kitware.com/cmake/cmake/issues/18771)
		# set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-interface;${NamePy}")

		# set_property(SOURCE ${SwigFile} PROPERTY SWIG_MODULE_NAME ${NamePy}) prevents rebuild every time make is called (? scrgiorgio sommented for now)
		set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
		
		swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile})
		target_compile_definitions(${NamePy} PRIVATE SWIG_TYPE_TABLE=OpenVisus)

		target_link_libraries(${NamePy} PUBLIC ${WrappedLib})
		set_target_properties(${NamePy} PROPERTIES FOLDER Swig/)

		if (WIN32)
			target_compile_definitions(${NamePy} PRIVATE /W0) # disable warnings
		else()
			set_target_properties(${NamePy} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
		endif()
	
		# swig generated *.py file and *.so must be in the same OpenVisus/ root directoryc otherwise it won't work (since swig auto-generate "from . import _VisusKernelPy") 
		set_target_properties(${NamePy} PROPERTIES 
			LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME} 
			RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}
			ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/swig) 

	endif()

endmacro()

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

	add_custom_target(CopyRootFiles ALL)

	CopyToBinaryDirectory(TARGET CopyRootFiles FILES     LICENSE                    DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyRootFiles FILES     README.md                  DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyRootFiles FILES     datasets/visus.config      DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyRootFiles FILES     ${__qt_version_filename__} DESTINATION ./)
	CopyToBinaryDirectory(TARGET CopyRootFiles FILES     quick_tour.ipynb           DESTINATION ./)

	CopyToBinaryDirectory(TARGET CopyRootFiles DIRECTORY Copyrights                 DESTINATION ./Copyrights)
	CopyToBinaryDirectory(TARGET CopyRootFiles DIRECTORY Samples                    DESTINATION ./Samples)
	CopyToBinaryDirectory(TARGET CopyRootFiles DIRECTORY datasets                   DESTINATION ./datasets)
	CopyToBinaryDirectory(TARGET CopyRootFiles DIRECTORY scripts                    DESTINATION ./scripts)

	# copy needed windows *.dll
	if (WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${ConfigName}/${PROJECT_NAME}/bin")
		include(InstallRequiredSystemLibraries)
		CopyToBinaryDirectory(TARGET CopyRootFiles FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ./bin/)
	endif()
	
endif()


# configure-step i.e. force use of PyQt to mix Qt5 and PyyQt5 ***
if (VISUS_PYTHON AND VISUS_GUI)
	add_custom_target(CONFIGURE ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/${ConfigName}/OpenVisus/.. ${Python_EXECUTABLE} -u -m OpenVisus use-pyqt5)
	set_property(TARGET CONFIGURE PROPERTY FOLDER CMakePredefinedTargets)
endif()


