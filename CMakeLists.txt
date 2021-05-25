CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW) # qt-related
endif()

if (POLICY CMP0071)
  cmake_policy(SET CMP0071 NEW) # qt-related
endif()

if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD) # swig related 
endif()

if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD)  # swig related
endif()

if (POLICY CMP0087)
  cmake_policy(SET CMP0087 NEW)  # cmake generator expression in install
endif()

# must be before PROJECT
set_property(GLOBAL PROPERTY USE_FOLDERS ON)  

# must be before PROJECT
SET(MSVC_INCREMENTAL_DEFAULT OFF) 


# ////////////////////////////////////////////////////////////////////////
macro(SetupCompiler)

	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	set(CMAKE_CXX_STANDARD              11)
	set(CMAKE_CXX_STANDARD_REQUIRED     ON)

	string( TOLOWER "${CMAKE_CXX_COMPILER_ID}" COMPILER_ID )
	MESSAGE(STATUS "Detected compiler with COMPILER_ID==${COMPILER_ID}")

	if (MSVC)
		MESSAGE(STATUS "Detected visual studio (C++ __MSVC_VER - WIN32)")

		# enable linking fo static libraries
		if (NOT BUILD_SHARED_LIBS)
			add_compile_options("$<$<CONFIG:DEBUG>:/MTd>")
			add_compile_options("$<$<CONFIG:RELEASE>:/MT>")
		endif()


	elseif (COMPILER_ID MATCHES ".*clang")
		set(CLANG 1)
		MESSAGE(STATUS "Detected apple clang (C++: __clang__ __APPLE__)")
		set(CMAKE_MACOSX_BUNDLE TRUE)
		set(CMAKE_MACOSX_RPATH  TRUE)

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

endmacro()

# ///////////////////////////////////////////////////////////
macro(DisableWarning Sources)
	if (MSVC)
		set_source_files_properties(${Sources} PROPERTIES COMPILE_FLAGS "/W0")
	else()
		set_source_files_properties(${Sources} PROPERTIES COMPILE_FLAGS "-w")
	endif()
endmacro()

# ///////////////////////////////////////////////////////////
macro(InstallDlls pattern)
	if (WIN32)
		file(GLOB __files__ ${pattern})
		foreach(__it__ ${__files__})
			get_filename_component(__path__ "${__it__}"  REALPATH)
			INSTALL(FILES ${__path__} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/bin/)
		endforeach()
	endif()
endmacro()

# ///////////////////////////////////////////////////////////
macro(InstallExtraDlls)
	if (WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS 1)
		include(InstallRequiredSystemLibraries)
		InstallDlls("${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")

		# there are some extra DLL that cmake misses to add
		list(GET CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS 0 __first__)
		InstallDlls("${__first__}/../*.dll") 
	endif()
endmacro()

# ///////////////////////////////////////////////////////////
macro(InstallOsprayDlls)
	if (WIN32 AND VISUS_OSPRAY)
		InstallDlls("${ospray_DIR}/../../../bin/*.dll")
	endif()
endmacro()

PROJECT(OpenVisus) 

if (VISUS_MINIMAL)
	set(VISUS_DEFAULT_NET         OFF)
	set(VISUS_DEFAULT_IMAGE       OFF)
	set(VISUS_DEFAULT_PYTHON      OFF)
	set(VISUS_DEFAULT_GUI         OFF)
	set(VISUS_DEFAULT_MODVISUS    OFF)
	set(VISUS_DEFAULT_OSPRAY      OFF)
	set(VISUS_DEFAULT_SLAM        OFF) 
	set(DEFAULT_BUILD_SHARED_LIBS OFF)
else()
	set(VISUS_DEFAULT_NET         ON)
	set(VISUS_DEFAULT_IMAGE       ON)
	set(VISUS_DEFAULT_PYTHON      ON)
	set(VISUS_DEFAULT_GUI         ON)
	set(VISUS_DEFAULT_SLAM        ON) 
	set(DEFAULT_BUILD_SHARED_LIBS ON)

	set(VISUS_DEFAULT_OSPRAY  OFF)
	if(WIN2 AND EXISTS "${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/README.txt")
		set(VISUS_DEFAULT_OSPRAY  ON)
	endif()

	set(VISUS_DEFAULT_MODVISUS OFF)
	if (NOT APPLE AND NOT WIN32)
		set(VISUS_DEFAULT_MODVISUS ON)
	endif()
	
endif()

option(BUILD_SHARED_LIBS "Build using shared libraries" ${DEFAULT_BUILD_SHARED_LIBS})
option(VISUS_NET         "Enable VISUS_NET"             ${VISUS_DEFAULT_NET})
option(VISUS_IMAGE       "Enable VISUS_IMAGE"           ${VISUS_DEFAULT_IMAGE})
option(VISUS_PYTHON      "Enable VISUS_PYTHON"          ${VISUS_DEFAULT_PYTHON})
option(VISUS_GUI         "Enable VISUS_GUI"             ${VISUS_DEFAULT_GUI})
option(VISUS_MODVISUS    "Enable VISUS_MODVISUS"        ${VISUS_DEFAULT_MODVISUS})
option(VISUS_SLAM        "Enable VISUS_SLAM"            ${VISUS_DEFAULT_SLAM})
option(VISUS_OSPRAY      "Enable VISUS_OSPRAY"          ${VISUS_DEFAULT_OSPRAY})

MESSAGE(STATUS "BUILD_SHARED_LIBS  ${BUILD_SHARED_LIBS}")
MESSAGE(STATUS "VISUS_NET          ${VISUS_NET}")
MESSAGE(STATUS "VISUS_IMAGE        ${VISUS_IMAGE}")
MESSAGE(STATUS "VISUS_PYTHON       ${VISUS_PYTHON}")
MESSAGE(STATUS "VISUS_GUI          ${VISUS_GUI}")
MESSAGE(STATUS "VISUS_MODVISUS     ${VISUS_MODVISUS}")
MESSAGE(STATUS "VISUS_SLAM         ${VISUS_SLAM}")
MESSAGE(STATUS "VISUS_OSPRAY       ${VISUS_OSPRAY}")

SetupCompiler()

include(FindPackageHandleStandardArgs)

if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
endif()

if (VISUS_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
	MESSAGE(STATUS "Python_EXECUTABLE ${Python_EXECUTABLE}")
	MESSAGE(STATUS "Python_VERSION    ${Python_VERSION}")
endif()

# for windows do:
# clone https://github.com/sci-visus/ospray_win.git ./ExternalLibs/ospray_win
if (VISUS_OSPRAY)
	set(OSPCOMMON_TBB_ROOT "${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/tbb")
	find_package(ospray 2.0   REQUIRED HINTS ${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/ospray/lib/cmake/ospray-2.1.1)
	find_package(ospcommon    REQUIRED HINTS ${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/ospcommon/lib/cmake/ospcommon-1.3.0)
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////
# install step

set (CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE STRING "CMAKE_INSTALL_PREFIX" FORCE)

if(VISUS_GUI)
	set(__qt_ver__ ${CMAKE_BINARY_DIR}/QT_VERSION)
	file(GENERATE OUTPUT ${__qt_ver__} CONTENT "${Qt5Core_VERSION_STRING}")
	INSTALL(FILES ${__qt_ver__} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)
endif()

INSTALL(FILES     LICENSE                                 DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)
INSTALL(FILES     README.md                               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)
INSTALL(FILES     OpenVisusConfig.cmake                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/lib/cmake/${PROJECT_NAME}/)
INSTALL(FILES     datasets/visus.config                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)
INSTALL(DIRECTORY datasets                                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)

INSTALL(FILES     Samples/simple_query/CMakeLists.txt     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/Samples/simple_query/)
INSTALL(FILES     Samples/simple_query/main.cpp           DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/Samples/simple_query/)
INSTALL(FILES     Samples/simple_viewer2d/CMakeLists.txt  DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/Samples/simple_viewer2d/)
INSTALL(FILES     Samples/simple_viewer2d/main.cpp        DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/Samples/simple_viewer2d/)

if (VISUS_PYTHON)
	FILE(GLOB PyFiles Libs/swig/*.py)
	INSTALL(FILES     ${PyFiles}                          DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)
	INSTALL(DIRECTORY Samples/jupyter                     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/Samples/)
	INSTALL(DIRECTORY Samples/python                      DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/Samples/)
endif()

INSTALL(DIRECTORY Copyrights                              DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/)

INSTALL(DIRECTORY Libs/Kernel/include/Visus               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/include/Kernel/)
INSTALL(DIRECTORY Libs/Db/include/Visus                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/include/Db/)
INSTALL(DIRECTORY Libs/Dataflow/include/Visus             DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/include/Dataflow/)
INSTALL(DIRECTORY Libs/Nodes/include/Visus                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/include/Nodes/)
INSTALL(DIRECTORY Libs/Gui/include/Visus                  DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME}/include/Gui/)

InstallExtraDlls()
InstallOsprayDlls()

