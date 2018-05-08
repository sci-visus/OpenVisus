
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

# enable/disable gui-stuff (i.e. Qt dependent)
option(VISUS_GUI "Enable gui" TRUE)  

IF (VISUS_GUI)
  find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL PrintSupport REQUIRED)
endif()

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/ExternalLibs/python27)
  set(NUMPY_FOUND 0)
elif(PYTHON_VERSION) # this is for anaconda/python 2.7 installations
  find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
  find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)
  find_package(NumPy) # better to have it
else()
  find_package(PythonInterp 3 REQUIRED)
  find_package(PythonLibs   3 REQUIRED)
  find_package(NumPy) # better to have it
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# only if I'm compiling OpenVisus not as submodule...
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)

	# samples
	add_subdirectory(Samples)

	# testing
	enable_testing()
	AddPythonTest(VisusTestPyDataflow ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Dataflow.py ${CMAKE_CURRENT_SOURCE_DIR})
	AddPythonTest(VisusTestPyArray    ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Array.py    ${CMAKE_CURRENT_SOURCE_DIR})
	AddPythonTest(VisusTestPyIdx      ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py      ${CMAKE_CURRENT_SOURCE_DIR})
	add_test(NAME VisusTestIdx WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND visus --test-idx --max-seconds 300)
	
	# external app
	AddExternalApp(external_simple_query ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query ${CMAKE_BINARY_DIR}/Samples/simple_query "")

	if (VISUS_GUI)
		AddExternalApp(external_simple_viewer2d ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d ${CMAKE_BINARY_DIR}/Samples/simple_viewer2d "-DQt5_DIR='${Qt5_DIR}'")
	endif()

endif()
