
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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

include(FindPackageHandleStandardArgs)

# enable parallel building
set(CMAKE_NUM_PROCS 8)          

# use folders to organize projects                           
set_property(GLOBAL PROPERTY USE_FOLDERS ON)       

# save libraries and binaries in the same directory        
set(EXECUTABLE_OUTPUT_PATH  ${CMAKE_BINARY_DIR})           
set(LIBRARY_OUTPUT_PATH     ${CMAKE_BINARY_DIR})

# shared/static linking
option(BUILD_SHARED_LIBS "Build the shared library" TRUE)  

# enable/disable gui-stuff (i.e. Qt dependent)
option(VISUS_GUI "Enable gui" TRUE)  

IF (VISUS_GUI)
  find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL PrintSupport REQUIRED)
endif()

include(VisusMacros)

SetupCompiler()
SetupOpenMP()

if (EXISTS ${CMAKE_SOURCE_DIR}/ExternalLibs/python27)
  set(NUMPY_FOUND 0)
else()
  find_package(PythonInterp 3 REQUIRED)
  find_package(PythonLibs   3 REQUIRED)
  find_package(NumPy) # better to have it
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)
add_subdirectory(Samples)

# ///////////////////////////////////////////////////
enable_testing()

AddPythonTest(VisusTestPyDataflow ${CMAKE_SOURCE_DIR}/Samples/python/Dataflow.py)
AddPythonTest(VisusTestPyArray  ${CMAKE_SOURCE_DIR}/Samples/python/Array.py)
AddPythonTest(VisusTestPyIdx    ${CMAKE_SOURCE_DIR}/Samples/python/Idx.py)

add_test(NAME VisusTestIdx WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMAND visus --test-idx --max-seconds 300)

# ///////////////////////////////////////////////////
AddExternalApp(external_simple_query Samples/simple_query "")

if (VISUS_GUI)
	AddExternalApp(external_simple_viewer2d Samples/simple_viewer2d "-DQt5_DIR='${Qt5_DIR}'")
endif()

