CMAKE_MINIMUM_REQUIRED(VERSION 3.0)


# /////////////////////////////////////////////////////////////////////
macro(DetectAndSetupCompiler)

	string( TOLOWER "${CMAKE_CXX_COMPILER_ID}" COMPILER_ID )
	MESSAGE(STATUS "COMPILER_ID ${COMPILER_ID}")

	# detect compiler
	if (MSVC)
		MESSAGE(STATUS "Detected visual studio (C++ __MSVC_VER - WIN32)")
	elseif (COMPILER_ID MATCHES ".*clang")
		set(CLANG 1)
		MESSAGE(STATUS "Detected apple clang (C++: __clang__ __APPLE__)")
	elseif (MINGW)
		set(GNUC  1)
		MESSAGE(STATUS "Detected GNU on windows (C++ __GNUC__ WIN32)")
	else()
		set(GNUC  1)
		if (MINGW)
			MESSAGE(STATUS "Detected GNU on windows (C++ __GNUC__ WIN32)")
			set(WIN32 1)
		elseif (APPLE)
			MESSAGE(STATUS "Detected GNU on apple (C++ __GNUC__ __APPLE__)")
			set(APPLE 1)
		else()
			MESSAGE(STATUS "Detected GNU on unix (C++ __GNUC__)")
      EXECUTE_PROCESS (COMMAND ${CMAKE_C_COMPILER} -print-file-name=libc.so.6 OUTPUT_VARIABLE GLIBC OUTPUT_STRIP_TRAILING_WHITESPACE)
      GET_FILENAME_COMPONENT (GLIBC ${GLIBC} REALPATH)
      MESSAGE(STATUS "GLIBC ${GLIBC}")
		endif()
	
		message(STATUS "CMAKE_SYSTEM_PROCESSOR ${CMAKE_SYSTEM_PROCESSOR}")
	
	endif()

	set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	set(CMAKE_CXX_STANDARD_REQUIRED     ON)
	
	if (VISUS_IDX2)
		set(CMAKE_CXX_STANDARD 17)
	else()
		set(CMAKE_CXX_STANDARD 11)
	endif()
	
	#minimum support is Vista
	if (MINGW)
		add_compile_options(-D_WIN32_WINNT=0x0600)
		add_compile_options(-DWINVER=0x0600)
	endif()

	if (CLANG)
		set(CMAKE_MACOSX_BUNDLE TRUE)
		set(CMAKE_MACOSX_RPATH  TRUE)
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
		file(GLOB __dlls__ ${pattern})
		foreach(__it__ ${__dlls__})
			get_filename_component(__path__ "${__it__}"  REALPATH)
			INSTALL(FILES ${__path__} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/bin/)
		endforeach()
	endif()
endmacro()

# ///////////////////////////////////////////////////////////
macro(InstallRunTimeDlls)
	if (WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_NO_WARNINGS 1)
		include(InstallRequiredSystemLibraries)
		InstallDlls("${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}")

		# there are some extra DLL that cmake misses to add
		list(GET CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS 0 __first_dll__)
		InstallDlls("${__first_dll__}/../*.dll") 
	endif()
endmacro()

# ///////////////////////////////////////////////////////////
macro(InstallOsprayDlls)
	if (WIN32 AND VISUS_OSPRAY)
		InstallDlls("${ospray_DIR}/../../../bin/*.dll")
	endif()
endmacro()

if (POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW) # windows CMAKE_MSVC_RUNTIME_LIBRARY
endif()

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

# must be before PROJECT command
set_property(GLOBAL PROPERTY USE_FOLDERS ON) 

# must be before PROJECT command
SET(MSVC_INCREMENTAL_DEFAULT OFF) 


option(VISUS_MINIMAL      "Enable VISUS_MINIMAL"   OFF)


# this must be before project
if (VISUS_MINIMAL)
	MESSAGE(STATUS "Forcing static linking on MSVC")
	SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

PROJECT(OpenVisus) 

# todo this default for linux too
if (NOT APPLE AND NOT WIN32)
	set(VISUS_DEFAULT_MODVISUS ON)
endif()

# git clone https://github.com/sci-visus/ospray_win.git ./ExternalLibs/ospray_win
set(VISUS_DEFAULT_OSPRAY  OFF)
if(WIN2 AND EXISTS "${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/README.txt")
	set(VISUS_DEFAULT_OSPRAY ON)
endif()


option(BUILD_SHARED_LIBS  "Build shared libraries" ON)
option(VISUS_NET          "Enable VISUS_NET"       ON)
option(VISUS_IMAGE        "Enable VISUS_IMAGE"     ON)
option(VISUS_DATAFLOW     "Enable VISUS_DATAFLOW"  ON)
option(VISUS_PYTHON       "Enable VISUS_PYTHON"    ON)
option(VISUS_GUI          "Enable VISUS_GUI"       ON)
option(VISUS_SLAM         "Enable VISUS_SLAM"      ON)
option(VISUS_OSPRAY       "Enable VISUS_OSPRAY"    ${VISUS_DEFAULT_OSPRAY})
option(VISUS_MODVISUS     "Enable VISUS_MODVISUS"  OFF)
option(VISUS_HDF5         "Enable VISUS_HDF5"      OFF)
option(VISUS_WEAVING      "Enable VISUS_WEAVING"   OFF)
option(VISUS_IDX2         "Enable VISUS_IDX2"      OFF)

if (VISUS_MINIMAL)
	SET(BUILD_SHARED_LIBS OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_NET         OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_IMAGE       OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_DATAFLOW    OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_PYTHON      OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_GUI         OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_MODVISUS    OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_SLAM        OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_OSPRAY      OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_HDF5        OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_WEAVING     OFF CACHE BOOL "disabled" FORCE)
	SET(VISUS_IDX2        OFF CACHE BOOL "disabled" FORCE)
endif()
	
MESSAGE(STATUS "BUILD_SHARED_LIBS  ${BUILD_SHARED_LIBS}")
MESSAGE(STATUS "VISUS_NET          ${VISUS_NET}")
MESSAGE(STATUS "VISUS_IMAGE        ${VISUS_IMAGE}")
MESSAGE(STATUS "VISUS_DATAFLOW     ${VISUS_DATAFLOW}")
MESSAGE(STATUS "VISUS_PYTHON       ${VISUS_PYTHON}")
MESSAGE(STATUS "VISUS_GUI          ${VISUS_GUI}")
MESSAGE(STATUS "VISUS_MODVISUS     ${VISUS_MODVISUS}")
MESSAGE(STATUS "VISUS_SLAM         ${VISUS_SLAM}")
MESSAGE(STATUS "VISUS_OSPRAY       ${VISUS_OSPRAY}")
MESSAGE(STATUS "VISUS_HDF5         ${VISUS_HDF5}")
MESSAGE(STATUS "VISUS_WEAVING      ${VISUS_WEAVING}")
MESSAGE(STATUS "VISUS_IDX2         ${VISUS_IDX2}")

# to call after the configuration 
DetectAndSetupCompiler()

include(FindPackageHandleStandardArgs)

if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
endif()

if (VISUS_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
	MESSAGE(STATUS "Python_EXECUTABLE   ${Python_EXECUTABLE}")
	MESSAGE(STATUS "Python_VERSION      ${Python_VERSION}")
	MESSAGE(STATUS "Python_INCLUDE_DIRS ${Python_INCLUDE_DIRS}")
	MESSAGE(STATUS "Python_LIBRARIES    ${Python_LIBRARIES}")
endif()

if (VISUS_OSPRAY)
	set(OSPCOMMON_TBB_ROOT "${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/tbb")
	find_package(ospray 2.0   REQUIRED HINTS ${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/ospray/lib/cmake/ospray-2.1.1)
	find_package(ospcommon    REQUIRED HINTS ${CMAKE_SOURCE_DIR}/ExternalLibs/ospray_win/ospcommon/lib/cmake/ospcommon-1.3.0)
endif()

if (VISUS_HDF5)
	find_package(HDF5 REQUIRED)
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)

if (NOT VISUS_MINIMAL)
	add_subdirectory(Executable)
endif()

# ///////////////////////////////////////////////////////////////////////////
# install step (NOTE: there is no need to install binaries/libraries since they are already in correct position)
# ///////////////////////////////////////////////////////////////////////////

set (CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE STRING "CMAKE_INSTALL_PREFIX" FORCE)

# create a QT_VERSION that will be needed for configure step on user machine
if(VISUS_GUI)
	set(__qt_ver__ ${CMAKE_BINARY_DIR}/QT_VERSION)
	file(GENERATE OUTPUT ${__qt_ver__} CONTENT "${Qt5Core_VERSION_STRING}")
	INSTALL(FILES ${__qt_ver__} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
endif()

INSTALL(FILES     LICENSE                                 DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
INSTALL(FILES     README.md                               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
INSTALL(FILES     OpenVisusConfig.cmake                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/lib/cmake/OpenVisus/)
INSTALL(FILES     datasets/visus.config                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
INSTALL(DIRECTORY datasets                                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
INSTALL(DIRECTORY Copyrights                              DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)

INSTALL(FILES     Samples/simple_query/CMakeLists.txt     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_query/)
INSTALL(FILES     Samples/simple_query/main.cpp           DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_query/)
INSTALL(FILES     Samples/simple_viewer2d/CMakeLists.txt  DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_viewer2d/)
INSTALL(FILES     Samples/simple_viewer2d/main.cpp        DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_viewer2d/)

INSTALL(DIRECTORY Libs/Kernel/include/Visus               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Kernel/)
INSTALL(DIRECTORY Libs/Db/include/Visus                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Db/)
INSTALL(DIRECTORY Libs/Dataflow/include/Visus             DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Dataflow/)
INSTALL(DIRECTORY Libs/Nodes/include/Visus                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Nodes/)
INSTALL(DIRECTORY Libs/Gui/include/Visus                  DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Gui/)

if (VISUS_PYTHON)
	FILE(GLOB PyFiles Libs/swig/*.py)
	INSTALL(FILES     ${PyFiles}                          DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(DIRECTORY Samples/jupyter                     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/)
	INSTALL(DIRECTORY Samples/python                      DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/)
endif()

# some dlls for windows
InstallRunTimeDlls()
InstallOsprayDlls()






