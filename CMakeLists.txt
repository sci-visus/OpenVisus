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

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  

PROJECT(OpenVisus) 

if (NOT CMAKE_CONFIGURATION_TYPES)
	if(NOT DEFINED CMAKE_BUILD_TYPE)
	  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
	endif()
endif()

# //////////////////////////////////////////////////////////////
include(CMake/VisusMacros.cmake)

include(FindPackageHandleStandardArgs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

option(BUILD_SHARED_LIBS  "Build the shared library" ON)  
option(VISUS_COMPRESSION  "Enable compression"       ON)  
option(VISUS_NET          "Enable net"               ON) 
option(VISUS_IMAGE        "Enable image"             ON)  
option(VISUS_GUI          "Enable gui"               ON)  
option(VISUS_OSPRAY       "Enable ospray"            OFF)

message(STATUS "BUILD_SHARED_LIBS  ${BUILD_SHARED_LIBS}")
message(STATUS "VISUS_COMPRESSION  ${VISUS_COMPRESSION}") 
message(STATUS "VISUS_NET          ${VISUS_NET}") 
message(STATUS "VISUS_IMAGE        ${VISUS_IMAGE}") 
message(STATUS "VISUS_GUI          ${VISUS_GUI}") 
message(STATUS "VISUS_OSPRAY       ${VISUS_OSPRAY}") 

FindPythonLibrary()

if (VISUS_OSPRAY)
	find_package(ospray REQUIRED)
endif()

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

endif()

add_subdirectory(Libs)
add_subdirectory(Executable)

if (CMAKE_CONFIGURATION_TYPES)
	set(OpenVisus_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>/site-packages/${CMAKE_PROJECT_NAME})
else()
	set(OpenVisus_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/site-packages/${CMAKE_PROJECT_NAME})
endif()


# //////////////////////////////////////////////////////////////
file(GENERATE OUTPUT ${OpenVisus_DIR}/PYTHON_VERSION CONTENT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")

if (VISUS_GUI)
	file(GENERATE OUTPUT ${OpenVisus_DIR}/QT_VERSION CONTENT ${Qt5Core_VERSION_STRING})
endif()

file(GLOB PyFiles ${CMAKE_CURRENT_SOURCE_DIR}/CMake/*.py)
foreach(src_filename ${PyFiles})
	get_filename_component(src_basename ${src_filename} NAME)
	file(GENERATE OUTPUT ${OpenVisus_DIR}/${src_basename} INPUT ${src_filename})
endforeach()


# //////////////////////////////////////////////////////////////
# install
if (1)

	# CMAKE_INSTALL_PREFIX is not used at all!
	mark_as_advanced(CMAKE_INSTALL_PREFIX)

	if (CMAKE_CONFIGURATION_TYPES)
		set(INSTALL_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_CONFIG_NAME}/site-packages/${CMAKE_PROJECT_NAME})
	else()
		set(INSTALL_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/site-packages/${CMAKE_PROJECT_NAME})
	endif()

	install(FILES          LICENSE                                       DESTINATION ${INSTALL_DIRECTORY})
	install(FILES          README.md                                     DESTINATION ${INSTALL_DIRECTORY})
	install(FILES          CMake/OpenVisusConfig.cmake                   DESTINATION ${INSTALL_DIRECTORY})
	install(FILES          datasets/visus.config                         DESTINATION ${INSTALL_DIRECTORY})
	install(DIRECTORY      Copyrights                                    DESTINATION ${INSTALL_DIRECTORY})
	install(DIRECTORY      Samples                                       DESTINATION ${INSTALL_DIRECTORY})
	install(DIRECTORY      CMake                                         DESTINATION ${INSTALL_DIRECTORY})
	install(DIRECTORY      datasets                                      DESTINATION ${INSTALL_DIRECTORY})

	install(DIRECTORY      Libs/Kernel/include/Visus                     DESTINATION ${INSTALL_DIRECTORY}/include/Kernel/)
	install(DIRECTORY      Libs/Dataflow/include/Visus                   DESTINATION ${INSTALL_DIRECTORY}/include/Dataflow/)
	install(DIRECTORY      Libs/Db/include/Visus                         DESTINATION ${INSTALL_DIRECTORY}/include/Db/)
	install(DIRECTORY      Libs/Idx/include/Visus                        DESTINATION ${INSTALL_DIRECTORY}/include/Idx/)
	install(DIRECTORY      Libs/Nodes/include/Visus                      DESTINATION ${INSTALL_DIRECTORY}/include/Nodes)

	if (VISUS_GUI)
		install(DIRECTORY   Libs/Gui/include/Visus                        DESTINATION ${INSTALL_DIRECTORY}/include/Gui)
		install(DIRECTORY   Libs/GuiNodes/include/Visus                   DESTINATION ${INSTALL_DIRECTORY}/include/GuiNodes)
		install(DIRECTORY   Libs/AppKit/include/Visus                     DESTINATION ${INSTALL_DIRECTORY}/include/AppKit)
	endif()

	if (WIN32)

		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	

		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION                                  ${INSTALL_DIRECTORY}/bin)
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}                DESTINATION ${INSTALL_DIRECTORY}/bin)	
	endif()

	macro(CMakePostInstallStep)

		get_filename_component(Qt5_HOME "${Qt5_DIR}/../../.." REALPATH)

		if (VISUS_GUI)
			install(CODE "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${INSTALL_DIRECTORY}/Deploy.py\"  CopyQt5Plugins     --qt5-home=\"${Qt5_HOME}\")")
		endif()

		install(CODE     "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${INSTALL_DIRECTORY}/Deploy.py\"  MakeSelfContained  --qt5-home=\"${Qt5_HOME}\")")
		install(CODE     "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${INSTALL_DIRECTORY}/Deploy.py\"  CreateScripts)")
	endmacro()

	add_subdirectory(CMake/PostInstall)

endif()


# //////////////////////////////////////////////////////////////
# *** dist (to call after INSTALL) ***
if (1)
	add_custom_target(dist 
		COMMAND ${PYTHON_EXECUTABLE} -u \"${CMAKE_CURRENT_SOURCE_DIR}/CMake/Deploy.py\" PythonDist)
	set_property(TARGET dist PROPERTY FOLDER CMakePredefinedTargets)
endif()


# //////////////////////////////////////////////////////////////
# *** testing (to call after INSTALL) ***
if (1)

	enable_testing()

	macro(AddOpenVisusTest Name)
		add_test(NAME ${Name} ${ARGN})
		set_property(TEST ${Name} APPEND PROPERTY ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1")
		set_property(TEST ${Name} APPEND PROPERTY ENVIRONMENT "PYTHONPATH=${OpenVisus_DIR}/..")
	endmacro()
	
	AddOpenVisusTest(VisusTestPyDataflow COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Dataflow.py")
	AddOpenVisusTest(VisusTestPyArray    COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Array.py")
	AddOpenVisusTest(VisusTestPyIdx      COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py")
	AddOpenVisusTest(VisusTestPyXIdx     COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py")

	# disabled: takes too much time, in case you modify some core IDX class you can run it manually
	# AddOpenVisusTest(NAME VisusTestIdx COMMAND "$<TARGET_FILE:visus>"     "${CMAKE_CURRENT_SOURCE_DIR}" --test-idx --max-seconds 300)
endif()


# //////////////////////////////////////////////////////////////
# *** external apps (dependent on install)
if (1)

	# external app
	macro(AddOpenVisusExternalApp SourceDir)

		get_filename_component(Name ${SourceDir} NAME_WE   )

		if (WIN32 OR APPLE)
			set(CMAKE_GENERATOR_ARGUMENT -G"${CMAKE_GENERATOR}")
		else()
			set(CMAKE_GENERATOR_ARGUMENT -G"\"${CMAKE_GENERATOR}\"")
		endif()

		if (CMAKE_CONFIGURATION_TYPES)
			set(BinaryDir ${CMAKE_BINARY_DIR}/$<CONFIG>/ExternalApps/${Name})
		else()
			set(BinaryDir ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/ExternalApps/${Name})
		endif()

		add_custom_target(${Name} 
			COMMAND "${CMAKE_COMMAND}"  "${CMAKE_GENERATOR_ARGUMENT}" -H"${SourceDir}/"  -B"${BinaryDir}/"  -DQt5_DIR="${Qt5_DIR}" -DOpenVisus_DIR=${OpenVisus_DIR}
			COMMAND "${CMAKE_COMMAND}"  --build "${BinaryDir}/" --config ${CMAKE_BUILD_TYPE})

		set_target_properties(${Name} PROPERTIES FOLDER CMakeTargets/)
	endmacro()

	AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d)
	endif()

endif()


