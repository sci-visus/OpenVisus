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

PROJECT(OpenVisus) 

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
option(VISUS_COMPRESSION  "Enable compression"       ON)  
option(VISUS_NET          "Enable net"               ON) 
option(VISUS_IMAGE        "Enable image"             ON)  
option(VISUS_GUI          "Enable gui"               ON)  
option(VISUS_OSPRAY       "Enable ospray"            OFF)
option(VISUS_TEST         "Enable test"              ON)

message(STATUS "BUILD_SHARED_LIBS  ${BUILD_SHARED_LIBS}")
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

file(GENERATE OUTPUT ${PYTHON_BINARY_DIR}/PYTHON_VERSION CONTENT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")


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

	file(GENERATE OUTPUT ${PYTHON_BINARY_DIR}/QT_VERSION CONTENT ${Qt5Core_VERSION_STRING})
	
endif()

if (VISUS_OSPRAY)
	find_package(ospray REQUIRED)
endif()

# copy py files to build
if (1)
	file(GLOB PyFiles ${CMAKE_CURRENT_SOURCE_DIR}/CMake/*.py)
	foreach(__src__ ${PyFiles})
		get_filename_component(__basename__ ${__src__} NAME)
		set(__dst__ "${PYTHON_BINARY_DIR}/${__basename__}")
		file(GENERATE OUTPUT ${__dst__} INPUT ${__src__})
	endforeach()
endif()

add_subdirectory(Libs)
add_subdirectory(Executable)


# //////////////////////////////////////////////////////////////
# install
if (1)

	install(FILES        LICENSE                                       DESTINATION OpenVisus)
	install(FILES        README.md                                     DESTINATION OpenVisus)
	install(FILES        CMake/OpenVisusConfig.cmake                   DESTINATION OpenVisus)
	install(FILES        datasets/visus.config                         DESTINATION OpenVisus)

	install(DIRECTORY    Copyrights                                    DESTINATION OpenVisus)
	install(DIRECTORY    Samples                                       DESTINATION OpenVisus)
	install(DIRECTORY    CMake                                         DESTINATION OpenVisus)
	install(DIRECTORY    datasets                                      DESTINATION OpenVisus)

	install(DIRECTORY    ${PYTHON_BINARY_DIR}/                         DESTINATION OpenVisus) 

	# includes
	install(DIRECTORY    Libs/Kernel/include/Visus                     DESTINATION OpenVisus/include/Kernel/)
	install(DIRECTORY    Libs/Dataflow/include/Visus                   DESTINATION OpenVisus/include/Dataflow/)
	install(DIRECTORY    Libs/Db/include/Visus                         DESTINATION OpenVisus/include/Db/)
	install(DIRECTORY    Libs/Idx/include/Visus                        DESTINATION OpenVisus/include/Idx/)
	install(DIRECTORY    Libs/Nodes/include/Visus                      DESTINATION OpenVisus/include/Nodes)

	if (VISUS_GUI)
		install(DIRECTORY Libs/Gui/include/Visus                       DESTINATION OpenVisus/include/Gui)
		install(DIRECTORY Libs/GuiNodes/include/Visus                  DESTINATION OpenVisus/include/GuiNodes)
		install(DIRECTORY Libs/AppKit/include/Visus                    DESTINATION OpenVisus/include/AppKit)
	endif()

	# in case some file have changed
	file(GLOB PyFiles ${CMAKE_CURRENT_SOURCE_DIR}/Libs/Kernel/src/*.py)
	install (FILES ${PyFiles} DESTINATION OpenVisus)

	if (WIN32)
		
		#macro(InstallBuildFiles Pattern Destination)
		#	install(CODE "
		#		FILE(GLOB __files__ ${CMAKE_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${Pattern})
		#		FILE(INSTALL \${__files__} DESTINATION \"${Destination}\")")
		#endmacro()
		#InstallBuildFiles(*.dll ${CMAKE_INSTALL_PREFIX}/OpenVisus/bin)

		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION OpenVisus/bin)
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}  DESTINATION OpenVisus/bin COMPONENT Libraries)	
	endif()

	macro(InstallPostInstallStep)

		get_filename_component(Qt5_HOME "${Qt5_DIR}/../../.." REALPATH)
		MESSAGE(STATUS "Qt5_HOME ${Qt5_HOME}")

		# copy qt plugins
		if (VISUS_GUI)
			install(CODE "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${CMAKE_INSTALL_PREFIX}/OpenVisus/Deploy.py\" CopyQt5Plugins    --qt5-home=\"${Qt5_HOME}\")")
		endif()

		# make self contained
		install(CODE     "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${CMAKE_INSTALL_PREFIX}/OpenVisus/Deploy.py\" MakeSelfContained --qt5-home=\"${Qt5_HOME}\")")

		# create scripts
		install(CODE     "execute_process(COMMAND \"${PYTHON_EXECUTABLE}\" \"${CMAKE_INSTALL_PREFIX}/OpenVisus/Deploy.py\" CreateScripts)")

	endmacro()
	add_subdirectory(CMake/PostInstall)

endif()


# ///////////////////////////////////////////////////////////////////////////////////////////////
add_custom_target(dist
	COMMAND ${PYTHON_EXECUTABLE} -u Deploy.py PythonDist
	WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/OpenVisus
)

if (WIN32)
	set_property(TARGET dist PROPERTY FOLDER CMakePredefinedTargets)
endif()


# //////////////////////////////////////////////////////////////
# *** testing (to call after INSTALL) ***
if (VISUS_TEST)

	# testing
	enable_testing()

	macro(AddOpenVisusTest Name)
		add_test(NAME ${Name} ${ARGN})
		set_property(TEST ${Name} APPEND PROPERTY ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1")
		set_property(TEST ${Name} APPEND PROPERTY ENVIRONMENT "PYTHONPATH=${CMAKE_INSTALL_PREFIX}")
	endmacro()
	
	AddOpenVisusTest(VisusTestPyDataflow COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_INSTALL_PREFIX}/OpenVisus/Samples/python/Dataflow.py")
	AddOpenVisusTest(VisusTestPyArray    COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_INSTALL_PREFIX}/OpenVisus/Samples/python/Array.py")
	AddOpenVisusTest(VisusTestPyIdx      COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_INSTALL_PREFIX}/OpenVisus/Samples/python/Idx.py")
	AddOpenVisusTest(VisusTestPyXIdx     COMMAND "${PYTHON_EXECUTABLE}"    "${CMAKE_INSTALL_PREFIX}/OpenVisus/Samples/python/Idx.py")

	# disabled: takes too much time, in case you modify some core IDX class you can run it manually
	# AddOpenVisusTest(NAME VisusTestIdx COMMAND "$<TARGET_FILE:visus>"         "${CMAKE_INSTALL_PREFIX}/OpenVisus" --test-idx --max-seconds 300)
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
			set(BinaryDir ${CMAKE_BINARY_DIR}/ExternalApps/${Name})
		endif()

		add_custom_target(${Name} 
			COMMAND "${CMAKE_COMMAND}"  "${CMAKE_GENERATOR_ARGUMENT}" -H"${SourceDir}/"  -B"${BinaryDir}/"  -DQt5_DIR="${Qt5_DIR}" -DOpenVisus_DIR=${CMAKE_INSTALL_PREFIX}/OpenVisus
			COMMAND "${CMAKE_COMMAND}"  --build "${BinaryDir}/" --config ${CMAKE_BUILD_TYPE})
		set_target_properties(${Name} PROPERTIES FOLDER CMakeTargets/)
	endmacro()


	AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d)
	endif()

endif()