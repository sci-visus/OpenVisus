CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

option(VISUS_MINIMAL ON)
if (VISUS_MINIMAL)

	PROJECT(OpenVisus) 

	set(CMAKE_CXX_STANDARD              11)
	set(CMAKE_CXX_STANDARD_REQUIRED     ON)

	file(GLOB_RECURSE Sources Libs/Kernel/*.cpp Libs/Kernel/*.c Libs/Db/*.cpp)
		
	if (CMAKE_CXX_COMPILER MATCHES ".*clang")
	  set(CMAKE_COMPILER_IS_CLANGXX 1)
	endif ()

	IF (APPLE AND CMAKE_COMPILER_IS_CLANGXX)
		file(GLOB SourcesMM Libs/Kernel/src/*.mm)
	endif()	
		
	add_library(VisusIO STATIC ${Sources} ${SourcesMM})
	target_compile_definitions(VisusIO  PUBLIC  VISUS_STATIC_KERNEL_LIB=1 VISUS_STATIC_DB_LIB=1)
	target_include_directories(VisusIO  
		PUBLIC  ${CMAKE_SOURCE_DIR}/Libs/Kernel/include
		PUBLIC  ${CMAKE_SOURCE_DIR}/Libs/Db/include
		PRIVATE ${CMAKE_SOURCE_DIR}/Libs/Kernel/src)

	if (WIN32)
		target_compile_options(VisusIO PRIVATE -DWIN32_LEAN_AND_MEAN -D_CRT_SECURE_NO_WARNINGS /bigobj)
		target_link_libraries(VisusIO  PRIVATE Psapi.lib Iphlpapi.lib DbgHelp.lib Ws2_32.lib)
		
	elseif (APPLE)
	
		if (CMAKE_COMPILER_IS_CLANGXX)
			target_compile_options(VisusIO PRIVATE  -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
			target_link_libraries(VisusIO  PRIVATE "-framework CoreFoundation" "-framework Foundation")
			set_target_properties(VisusIO  PROPERTIES XCODE_ATTRIBUTE_CLANG_LINK_OBJC_RUNTIME "NO")  # fixes problem of "_objc_readClassPair" missing
		endif()
		
	else()
		target_compile_options(VisusIO PRIVATE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE=1 -Wno-attributes)
		target_link_libraries(VisusIO  PRIVATE dl rt util pthread)
		
	endif()
	
	add_executable(kyle main.cpp)
	target_link_libraries(kyle PUBLIC VisusIO)
	
	return()

endif()

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
set(VISUS_DEFAULT_XIDX     ON) # not really used?
set(VISUS_DEFAULT_DATAFLOW ON)
set(VISUS_DEFAULT_PYTHON   ON)
set(VISUS_DEFAULT_GUI      ON)
set(VISUS_DEFAULT_MODVISUS ON)
set(VISUS_DEFAULT_OSPRAY   OFF)

if(EXISTS "${CMAKE_SOURCE_DIR}/Libs/slamcpp/slam.cpp")
	set(VISUS_DEFAULT_SLAM 1)
endif()

if (WIN32 OR APPLE)
	set(VISUS_DEFAULT_MODVISUS OFF)
endif()

option(VISUS_NET      "Enable VISUS_NET"      ${VISUS_DEFAULT_NET})
option(VISUS_IMAGE    "Enable VISUS_IMAGE"    ${VISUS_DEFAULT_IMAGE})
option(VISUS_XIDX     "Enable VISUS_XIDX"     ${VISUS_DEFAULT_XIDX})
option(VISUS_DATAFLOW "Enable VISUS_DATAFLOW" ${VISUS_DEFAULT_DATAFLOW})
option(VISUS_PYTHON   "Enable VISUS_PYTHON"   ${VISUS_DEFAULT_PYTHON})
option(VISUS_GUI      "Enable VISUS_GUI"      ${VISUS_DEFAULT_GUI})
option(VISUS_MODVISUS "Enable VISUS_MODVISUS" ${VISUS_DEFAULT_MODVISUS})
option(VISUS_SLAM     "Enable VISUS_SLAM"     ${VISUS_DEFAULT_SLAM})
option(VISUS_OSPRAY   "Enable VISUS_OSPRAY"   ${VISUS_DEFAULT_OSPRAY})

MESSAGE(STATUS "VISUS_NET      ${VISUS_NET}")
MESSAGE(STATUS "VISUS_IMAGE    ${VISUS_IMAGE}")
MESSAGE(STATUS "VISUS_XIDX     ${VISUS_XIDX}")
MESSAGE(STATUS "VISUS_DATAFLOW ${VISUS_DATAFLOW}")
MESSAGE(STATUS "VISUS_PYTHON   ${VISUS_PYTHON}")
MESSAGE(STATUS "VISUS_GUI      ${VISUS_GUI}")
MESSAGE(STATUS "VISUS_MODVISUS ${VISUS_MODVISUS}")
MESSAGE(STATUS "VISUS_SLAM     ${VISUS_SLAM}")
MESSAGE(STATUS "VISUS_OSPRAY   ${VISUS_OSPRAY}")

if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
endif()

if (VISUS_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# install step
if (1)

	INSTALL(FILES     LICENSE                     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(FILES     README.md                   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(FILES     datasets/visus.config       DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(FILES     OpenVisusConfig.cmake       DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/lib/cmake/OpenVisus/)


	if(VISUS_GUI)
		set(__qt_ver__ ${CMAKE_BINARY_DIR}/QT_VERSION)
		file(GENERATE OUTPUT ${__qt_ver__} CONTENT "${Qt5Core_VERSION_STRING}")
		INSTALL(FILES ${__qt_ver__}               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	endif()
	
	FILE(GLOB PyFiles Libs/swig/*.py)
	INSTALL(FILES     ${PyFiles}                              DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)

	INSTALL(FILES Samples/simple_query/CMakeLists.txt         DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_query/)
	INSTALL(FILES Samples/simple_query/main.cpp               DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_query/)

	INSTALL(FILES Samples/simple_viewer2d/CMakeLists.txt      DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_viewer2d/)
	INSTALL(FILES Samples/simple_viewer2d/main.cpp            DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/simple_viewer2d/)

	INSTALL(DIRECTORY Copyrights                              DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)
	INSTALL(DIRECTORY Samples/jupyter                         DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/)
	INSTALL(DIRECTORY Samples/python                          DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/Samples/)

	INSTALL(DIRECTORY datasets                                DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/)

	INSTALL(DIRECTORY Libs/Kernel/include/Visus   DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Kernel/)
	INSTALL(DIRECTORY Libs/XIdx/include/Visus     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/XIdx/)
	INSTALL(DIRECTORY Libs/Db/include/Visus       DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Db/)
	INSTALL(DIRECTORY Libs/Dataflow/include/Visus DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Dataflow/)
	INSTALL(DIRECTORY Libs/Nodes/include/Visus    DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Nodes/)
	INSTALL(DIRECTORY Libs/Gui/include/Visus      DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/Gui/)
	
	if (WIN32)
		# copy  *.dll
		include(InstallRequiredSystemLibraries)
		INSTALL(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/bin/)
	endif()
	
endif()



