
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

if(POLICY CMP0068)
  cmake_policy(SET CMP0068 NEW)
endif()

PROJECT(ViSUS)

include(CMake/VisusMacros.cmake)
SetupCMake()

include(FindPackageHandleStandardArgs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

# shared/static linking
option(BUILD_SHARED_LIBS "Build the shared library" TRUE)  

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(VISUS_IS_SUBMODULE 0)
	set(CMAKE_FOLDER_PREFIX "")
else()
	set(VISUS_IS_SUBMODULE 1)
	SET(CMAKE_FOLDER_PREFIX OpenVisus/)
endif()


# enable/disable install
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)
endif()

# enable/disable gui-stuff (i.e. Qt dependent)
option(VISUS_GUI "Enable gui" TRUE)  

IF (VISUS_GUI)

	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL PrintSupport REQUIRED)

	if (WIN32)
		string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
	endif()
  
	get_target_property(QMakeLocation Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(QtBinPath "${QMakeLocation}" DIRECTORY)

  if (WIN32)
    find_program(DEPLOYQT windeployqt.exe HINTS "${QtBinPath}")
  elseif(APPLE)
    find_program(DEPLOYQT macdeployqt     HINTS "${QtBinPath}")
  else()
	  # no official deploy for linux, only an unofficial one linuxdeployqt
  endif()

endif()

# find Python
set(PYTHON_VERSION 3)
find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)
find_package(NumPy REQUIRED) 

set(VISUS_SELF_CONTAINED 0)
if (WIN32 AND NOT CMAKE_TOOLCHAIN_FILE)
  set(VISUS_SELF_CONTAINED 1)
  message("Compiling self contained OpenVisus")
endif()

if (VISUS_SELF_CONTAINED)
	add_subdirectory(ExternalLibs)
endif()

add_subdirectory(Libs)
add_subdirectory(Executable)

# *** samples and external apps ***
if (NOT VISUS_IS_SUBMODULE)

	# samples
	add_subdirectory(Samples)

	# testing
	enable_testing()

  AddCTest(VisusTestPyDataflow ${PYTHON_EXECUTABLE}  ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Samples/python/Dataflow.py)
  AddCTest(VisusTestPyArray    ${PYTHON_EXECUTABLE}  ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Samples/python/Array.py)
  AddCTest(VisusTestPyIdx      ${PYTHON_EXECUTABLE}  ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/Samples/python/Idx.py)
	
  # disabled: takes too much time, in case you modify some core IDX class you can run it manually
  # AddCTest(VisusTestIdx      $<TARGET_FILE:visus> ${CMAKE_SOURCE_DIR}  --test-idx --max-seconds 300)
	
	# external app
	AddExternalApp(external_simple_query ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query ${CMAKE_BINARY_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddExternalApp(external_simple_viewer2d ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d ${CMAKE_BINARY_DIR}/Samples/simple_viewer2d)
	endif()

endif()

InstallVisus()

