
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

PROJECT(ViSUS)

include(CMake/VisusMacros.cmake)
SetupCMake()

include(FindPackageHandleStandardArgs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

# shared/static linking
option(BUILD_SHARED_LIBS "Build the shared library" TRUE)  

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(VISUS_IS_SUBMODULE 0)
else()
	set(VISUS_IS_SUBMODULE 1)
endif()

# enable/disable install
if (NOT VISUS_IS_SUBMODULE)
	if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)
	endif()
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

	execute_process(COMMAND ${QMakeLocation} -query QT_INSTALL_PLUGINS  OUTPUT_VARIABLE QT_INSTALL_PLUGINS OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT EXISTS ${QT_INSTALL_PLUGINS})
		message(FATAL_ERROR "Failed to detect QT_INSTALL_PLUGINS")
	endif()
  
  if (WIN32)
    find_program(DEPLOYQT windeployqt.exe HINTS "${QtBinPath}")
  endif()

endif()

FindPython()

add_subdirectory(Libs)
add_subdirectory(Executable)

# only if I'm compiling OpenVisus not as submodule...
if (NOT VISUS_IS_SUBMODULE)

	# samples
	add_subdirectory(Samples)

	# testing
	enable_testing()
	AddPythonTest(VisusTestPyDataflow ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Dataflow.py ${CMAKE_CURRENT_SOURCE_DIR})
	AddPythonTest(VisusTestPyArray    ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Array.py    ${CMAKE_CURRENT_SOURCE_DIR})
	AddPythonTest(VisusTestPyIdx      ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py      ${CMAKE_CURRENT_SOURCE_DIR})
	add_test(NAME VisusTestIdx WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND visus --test-idx --max-seconds 300)
	
	# external app
	AddExternalApp(external_simple_query ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query ${CMAKE_BINARY_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddExternalApp(external_simple_viewer2d ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d ${CMAKE_BINARY_DIR}/Samples/simple_viewer2d)
	endif()

endif()


InstallVisus()