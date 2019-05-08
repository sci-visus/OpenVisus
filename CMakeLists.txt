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

MESSAGE(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")

# //////////////////////////////////////////////////////////////
include(CMake/VisusMacros.cmake)

include(FindPackageHandleStandardArgs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

option(BUILD_SHARED_LIBS     "Build the shared library" ON)  
option(VISUS_COMPRESSION     "Enable compression"       ON)  
option(VISUS_NET             "Enable net"               ON) 
option(VISUS_IMAGE           "Enable image"             ON)  
option(VISUS_GUI             "Enable gui"               ON)  
option(VISUS_OSPRAY          "Enable ospray"            OFF)
option(VISUS_MODVISUS        "Enable mod_visus"         ON)
option(VISUS_DISABLE_PYTHON  "Disable python"           OFF)  

message(STATUS "BUILD_SHARED_LIBS    ${BUILD_SHARED_LIBS}")
message(STATUS "VISUS_COMPRESSION    ${VISUS_COMPRESSION}") 
message(STATUS "VISUS_NET            ${VISUS_NET}") 
message(STATUS "VISUS_IMAGE          ${VISUS_IMAGE}") 
message(STATUS "VISUS_GUI            ${VISUS_GUI}") 
message(STATUS "VISUS_OSPRAY         ${VISUS_OSPRAY}") 
message(STATUS "VISUS_MODVISUS       ${VISUS_MODVISUS}") 
message(STATUS "VISUS_DISABLE_PYTHON ${VISUS_DISABLE_PYTHON}") 


if (VISUS_DISABLE_PYTHON)

	file (REMOVE ${OpenVisus_DIR}/PYTHON_VERSION)
	
	# i can use PYTHON_EXECUTABLE just for deployment
	SetIfNotDefined(PYTHON_VERSION 3)
	find_package(PythonInterp ${PYTHON_VERSION})
	message(STATUS "PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE}")	
	
else()
	FindPythonLibrary()
	file(GENERATE OUTPUT ${OpenVisus_DIR}/PYTHON_VERSION CONTENT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
endif()

if (VISUS_OSPRAY)
	find_package(ospray REQUIRED)
endif()

if (VISUS_GUI)

	if (DEFINED Qt5_DIR)
	
		if (WIN32)
			string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
		endif()	
	
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR}                      NO_DEFAULT_PATH)
	elseif (APPLE AND EXISTS /usr/local/opt/qt/lib/cmake/Qt5)
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS /usr/local/opt/qt/lib/cmake/Qt5 NO_DEFAULT_PATH)
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
	set(OpenVisus_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>/site-packages/OpenVisus)
	string(REPLACE "$<CONFIG>" "\${CMAKE_INSTALL_CONFIG_NAME}" OpenVisusInstall_DIR "${OpenVisus_DIR}")
else()
	set(OpenVisus_DIR        ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus)
	set(OpenVisusInstall_DIR ${OpenVisus_DIR})
endif()

# //////////////////////////////////////////////////////////////

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

	install(FILES          LICENSE                                       DESTINATION ${OpenVisus_DIR})
	install(FILES          README.md                                     DESTINATION ${OpenVisus_DIR})
	install(FILES          CMake/OpenVisusConfig.cmake                   DESTINATION ${OpenVisus_DIR})
	install(FILES          datasets/visus.config                         DESTINATION ${OpenVisus_DIR})
	install(DIRECTORY      Copyrights                                    DESTINATION ${OpenVisus_DIR})
	install(DIRECTORY      Samples                                       DESTINATION ${OpenVisus_DIR})
	install(DIRECTORY      CMake                                         DESTINATION ${OpenVisus_DIR})
	install(DIRECTORY      datasets                                      DESTINATION ${OpenVisus_DIR})

	install(DIRECTORY      Libs/Kernel/include/Visus                     DESTINATION ${OpenVisus_DIR}/include/Kernel/)
	install(DIRECTORY      Libs/Dataflow/include/Visus                   DESTINATION ${OpenVisus_DIR}/include/Dataflow/)
	install(DIRECTORY      Libs/Db/include/Visus                         DESTINATION ${OpenVisus_DIR}/include/Db/)
	install(DIRECTORY      Libs/Idx/include/Visus                        DESTINATION ${OpenVisus_DIR}/include/Idx/)
	install(DIRECTORY      Libs/Nodes/include/Visus                      DESTINATION ${OpenVisus_DIR}/include/Nodes)

	if (VISUS_GUI)
		install(DIRECTORY    Libs/Gui/include/Visus                        DESTINATION ${OpenVisus_DIR}/include/Gui)
		install(DIRECTORY    Libs/GuiNodes/include/Visus                   DESTINATION ${OpenVisus_DIR}/include/GuiNodes)
		install(DIRECTORY    Libs/AppKit/include/Visus                     DESTINATION ${OpenVisus_DIR}/include/AppKit)
	endif()

	if (WIN32)

		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	

		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION                                  ${OpenVisus_DIR}/bin)
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}                DESTINATION ${OpenVisus_DIR}/bin)	
	endif()

	# MakeSelfContainedStep and CreateScriptsStep
	if (PYTHON_EXECUTABLE)
		
		macro(CMakePostInstallStep)
			get_filename_component(Qt5_HOME "${Qt5_DIR}/../../.." REALPATH)
			
			install(CODE "
				execute_process(COMMAND ${PYTHON_EXECUTABLE} ${OpenVisusInstall_DIR}/Deploy.py MakeSelfContainedStep --qt5-home=${Qt5_HOME} RESULT_VARIABLE retcode)
				if(NOT \"\${retcode}\" STREQUAL \"0\")
					message(FATAL_ERROR \"Fatal error executing make self contained\")
				endif()
			")
	
			install(CODE "
				execute_process(COMMAND ${PYTHON_EXECUTABLE} ${OpenVisusInstall_DIR}/Deploy.py CreateScriptsStep RESULT_VARIABLE retcode)
				if(NOT \"\${retcode}\" STREQUAL \"0\")
					message(FATAL_ERROR \"Fatal error executing configure step\")
				endif()
			")
		endmacro()	
	
		add_subdirectory(CMake/PostInstall)
	endif()

endif()


# //////////////////////////////////////////////////////////////
# *** dist (to call after INSTALL) ***
if (NOT VISUS_DISABLE_PYTHON)
	add_custom_target(dist COMMAND ${PYTHON_EXECUTABLE} -u \"${OpenVisus_DIR}/Deploy.py\" DistStep)
	set_property(TARGET dist PROPERTY FOLDER CMakePredefinedTargets)
endif()


# //////////////////////////////////////////////////////////////
# *** configure (to call after INSTALL, create scripts and forse use of PyQt) ***
if (NOT VISUS_DISABLE_PYTHON)
	add_custom_target(configure COMMAND ${PYTHON_EXECUTABLE} -u \"${OpenVisus_DIR}/Deploy.py\" configure)
	set_property(TARGET configure PROPERTY FOLDER CMakePredefinedTargets)
endif()


# //////////////////////////////////////////////////////////////
# *** RUN_TESTS (to call after INSTALL) ***
if (NOT VISUS_DISABLE_PYTHON)

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
# *** ExternalApps (dependent on install)
if (1)

	# external app
	macro(AddOpenVisusExternalApp SourceDir)

		get_filename_component(Name ${SourceDir} NAME_WE )

		if (CMAKE_CONFIGURATION_TYPES)
			set(CMAKE_GENERATOR_ARGUMENT -G"${CMAKE_GENERATOR}")
			set(BinaryDir ${CMAKE_BINARY_DIR}/$<CONFIG>/ExternalApps/${Name})
		else()
			set(CMAKE_GENERATOR_ARGUMENT -G"\"${CMAKE_GENERATOR}\"")
			set(BinaryDir ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/ExternalApps/${Name})
		endif()

		add_custom_target(${Name} 

			COMMAND "${CMAKE_COMMAND}" "${CMAKE_GENERATOR_ARGUMENT}" 
				-H"${SourceDir}/"  
				-B"${BinaryDir}/" 
				-DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} 
				-DPYTHON_LIBRARY=${PYTHON_LIBRARIES}
				-DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIRS}
				-DQt5_DIR="${Qt5_DIR}" 
				-DOpenVisus_DIR=${OpenVisus_DIR}

			COMMAND "${CMAKE_COMMAND}"  
				--build "${BinaryDir}/" 
				--config ${CMAKE_BUILD_TYPE}

		)

		set_target_properties(${Name} PROPERTIES FOLDER CMakeTargets/)
	endmacro()

	AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d)
	endif()

endif()


