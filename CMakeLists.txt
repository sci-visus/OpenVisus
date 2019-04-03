CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

# qt-related
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

# qt-related
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD)
endif()

if (POLICY CMP0022)
  cmake_policy(SET CMP0022  NEW) 
endif()	

if (NOT (${CMAKE_VERSION} VERSION_LESS "3.13"))
  cmake_policy(SET CMP0078 OLD) #controls names of SWIG targets
endif()

PROJECT(ViSUS) 

# //////////////////////////////////////////////////////////////
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)

if (WIN32)
	string(REPLACE "\\" "/" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif()

MESSAGE(STATUS "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")

include(CMake/VisusMacros.cmake)
SetupCommonCMake()

FindOpenMP()

include(FindPackageHandleStandardArgs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)


option(BUILD_SHARED_LIBS  "Build the shared library" ON)  
option(VISUS_PYTHON       "Enable python"            ON) 
option(VISUS_COMPRESSION  "Enable compression"       ON)  
option(VISUS_NET          "Enable net"               ON) 
option(VISUS_IMAGE        "Enable image"             ON)  
option(VISUS_GUI          "Enable gui"               ON)  
option(VISUS_OSPRAY       "Enable ospray"            OFF)
option(VISUS_TEST         "Enable test"              ON)

message(STATUS "BUILD_SHARED_LIBS  ${BUILD_SHARED_LIBS}") 
message(STATUS "VISUS_PYTHON       ${VISUS_PYTHON}") 
message(STATUS "VISUS_COMPRESSION  ${VISUS_COMPRESSION}") 
message(STATUS "VISUS_NET          ${VISUS_NET}") 
message(STATUS "VISUS_IMAGE        ${VISUS_IMAGE}") 
message(STATUS "VISUS_GUI          ${VISUS_GUI}") 
message(STATUS "VISUS_OSPRAY       ${VISUS_OSPRAY}") 
message(STATUS "VISUS_TEST         ${VISUS_TEST}") 

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(VISUS_IS_SUBMODULE 0)
else()
	set(VISUS_IS_SUBMODULE 1)
	set(CMAKE_FOLDER_PREFIX "")
endif()

FindPythonLibrary()

if (VISUS_GUI)
	IF (APPLE)
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED HINTS /usr/local/opt/qt/lib/cmake/Qt5)
	else()
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)
	endif()

	mark_as_advanced(Qt5Core_DIR)
	mark_as_advanced(Qt5Gui_DIR)
	mark_as_advanced(Qt5OpenGL_DIR)
	mark_as_advanced(Qt5Widgets_DIR)	

	if (WIN32)
		string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
	endif()	
	
	MESSAGE(STATUS "Qt5_DIR ${Qt5_DIR}")
	MESSAGE(STATUS "Qt5Core_VERSION_STRING ${Qt5Core_VERSION_STRING}")
	file(WRITE "${CMAKE_BINARY_DIR}/QT_VERSION" "${Qt5Core_VERSION_STRING}")	
	
endif()

if (VISUS_OSPRAY)
	find_package(ospray REQUIRED)
endif()

add_subdirectory(Libs)
add_subdirectory(Executable)


# //////////////////////////////////////////////////////////////
# *** testing and external apps ***
if (VISUS_TEST)

	# testing
	enable_testing()

	if (VISUS_PYTHON)
		AddCTest(VisusTestPyDataflow ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Dataflow.py)
		AddCTest(VisusTestPyArray    ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Array.py)
		AddCTest(VisusTestPyIdx      ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py)
		AddCTest(VisusTestPyXIdx     ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py)
	endif()

	# disabled: takes too much time, in case you modify some core IDX class you can run it manually
	if (0)
		AddCTest(VisusTestIdx      $<TARGET_FILE:visus> ${CMAKE_CURRENT_SOURCE_DIR}  --test-idx --max-seconds 300)
	endif()
	
	# external app
	AddExternalApp(simple_query ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query ${CMAKE_BINARY_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddExternalApp(simple_viewer2d ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d ${CMAKE_BINARY_DIR}/Samples/simple_viewer2d)
	endif()

endif()

# //////////////////////////////////////////////////////////////
# install
if (1)

	# swig generated files (important to be the first since I could have changed some *.py files meantime)
	InstallBuildFiles(*.py ./)

	file(GLOB CMakePySources CMake/*.py)
	
	install(DIRECTORY    CMake/PostInstall                             DESTINATION CMake)
	
	install(FILES        ${CMakePySources}                             DESTINATION .)
	install(FILES        LICENSE                                       DESTINATION .)
	install(FILES        README.md                                     DESTINATION .)
	install(FILES        CMake/__init__.py                             DESTINATION .)
	install(FILES        CMake/__main__.py                             DESTINATION .)
	install(FILES        CMake/configure.py                            DESTINATION .)
	install(FILES        CMake/OpenVisus.py                            DESTINATION .)
	install(FILES        CMake/OpenVisusConfig.cmake                   DESTINATION .)
	install(FILES        CMake/VisusMacros.cmake                       DESTINATION CMake)
	install(DIRECTORY    Copyrights                                    DESTINATION .)
	install(DIRECTORY    Samples                                       DESTINATION .)
	
	install(FILES        datasets/visus.config                         DESTINATION .)
	install(DIRECTORY    datasets/cat                                  DESTINATION datasets)
	install(DIRECTORY    datasets/midx                                 DESTINATION datasets)
	
	install(DIRECTORY    Libs/Kernel/include/Visus                     DESTINATION include/Kernel/)
	install(DIRECTORY    Libs/Dataflow/include/Visus                   DESTINATION include/Dataflow/)
	install(DIRECTORY    Libs/Db/include/Visus                         DESTINATION include/Db/)
	install(DIRECTORY    Libs/Idx/include/Visus                        DESTINATION include/Idx/)
	install(DIRECTORY    Libs/Nodes/include/Visus                      DESTINATION include/Nodes)
	
	if (VISUS_PYTHON)
		file(WRITE    "${CMAKE_BINARY_DIR}/PYTHON_VERSION" "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
		install(FILES "${CMAKE_BINARY_DIR}/PYTHON_VERSION" DESTINATION .)
	endif()

	if (VISUS_GUI)
		install(FILES "${CMAKE_BINARY_DIR}/QT_VERSION" DESTINATION .)
	endif()
	
	if (VISUS_OSPRAY)
		install(FILES "${OSPRAY_LIBRARIES}" DESTINATION bin)
	endif()
	
	if (WIN32)
	
		InstallBuildFiles(*.dll bin)
	
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin COMPONENT Libraries)	
		
	endif()

	if (VISUS_GUI)
		install(DIRECTORY Libs/Gui/include/Visus                DESTINATION include/Gui)
		install(DIRECTORY Libs/GuiNodes/include/Visus           DESTINATION include/GuiNodes)
		install(DIRECTORY Libs/AppKit/include/Visus             DESTINATION include/AppKit)
	endif()
	
	add_subdirectory(CMake/PostInstall)

endif()


add_custom_target(dist
	WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}
	COMMAND ${PYTHON_EXECUTABLE} -u configure.py cmake_dist_step
)

if (WIN32)
	set_property(TARGET dist PROPERTY FOLDER CMakePredefinedTargets)
endif()

