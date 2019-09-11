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

if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD) #controls names of SWIG targets
endif()

# see https://cmake.org/cmake/help/git-stage/policy/CMP0086.html
if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD)
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  

PROJECT(OpenVisus) 

if (NOT CMAKE_CONFIGURATION_TYPES)
	if(NOT DEFINED CMAKE_BUILD_TYPE)
	  set(CMAKE_BUILD_TYPE "Release")
	endif()
endif()

MESSAGE(STATUS "CMAKE_SYSTEM_NAME ${CMAKE_SYSTEM_NAME}")


# //////////////////////////////////////////////////////////////
include(CMake/VisusMacros.cmake)

include(FindPackageHandleStandardArgs)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

option(BUILD_SHARED_LIBS "Build the shared library"  ON)
option(VISUS_NET         "Enable net"                ON) 
option(VISUS_IMAGE       "Enable image"              ON)  
option(VISUS_PYTHON      "Enable python"             ON) 
option(VISUS_XIDX        "Enable VisusXIdx"          ON)
option(VISUS_DB          "Enable VisusDb"            ON)
option(VISUS_DATAFLOW    "Enable VisusDataflow"      ON)
option(VISUS_GUI         "Enable VisusGui"           ON)

include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(VISUS_IDX         "Enable VisusIdx"           ON  "VISUS_DB"                   OFF)
CMAKE_DEPENDENT_OPTION(VISUS_NODES       "Enable VisusNodes"         ON  "VISUS_DATAFLOW;VISUS_IDX;"  OFF)
CMAKE_DEPENDENT_OPTION(VISUS_GUINODES    "Enable VisusGuiNodes"      ON  "VISUS_GUI;VISUS_DATAFLOW"   OFF)
CMAKE_DEPENDENT_OPTION(VISUS_APPKIT      "Enable VisusAppKit"        ON  "VISUS_NODES;VISUS_GUINODES" OFF)
CMAKE_DEPENDENT_OPTION(VISUS_OSPRAY      "Enable ospray"             OFF "VISUS_GUINODES"             OFF)

message(STATUS "VISUS_NET         ${VISUS_NET}")
message(STATUS "VISUS_IMAGE       ${VISUS_IMAGE}")
message(STATUS "VISUS_PYTHON      ${VISUS_PYTHON}")
message(STATUS "VISUS_XIDX        ${VISUS_XIDX}")
message(STATUS "VISUS_DB          ${VISUS_DB}")
message(STATUS "VISUS_DATAFLOW    ${VISUS_DATAFLOW}")
message(STATUS "VISUS_GUI         ${VISUS_GUI}")
message(STATUS "VISUS_IDX         ${VISUS_IDX}")
message(STATUS "VISUS_NODES       ${VISUS_NODES}")
message(STATUS "VISUS_GUINODES    ${VISUS_GUINODES}")
message(STATUS "VISUS_OSPRAY      ${VISUS_OSPRAY}")
message(STATUS "VISUS_APPKIT      ${VISUS_APPKIT}")

# To activate the slam:
#    git submodule init 
#    git submodule update --init Libs/Slam

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Libs/Slam/CMakeLists.txt)
	set(VISUS_SLAM ON)
	message(STATUS "VISUS_SLAM        ${VISUS_SLAM}")
endif()


if (VISUS_PYTHON)
	FindPythonLibrary()
endif()

if (VISUS_OSPRAY)
	find_package(ospray REQUIRED)
endif()

if (VISUS_GUI)
	if (DEFINED Qt5_DIR)
		if (WIN32)
			string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
		endif()	
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
		
	elseif (APPLE AND EXISTS /usr/local/opt/qt/lib/cmake/Qt5)
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS /usr/local/opt/qt/lib/cmake/Qt5 NO_DEFAULT_PATH)
		
	else()
		find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)
		
 	endif()

	mark_as_advanced(Qt5Core_DIR)
	mark_as_advanced(Qt5Gui_DIR)
	mark_as_advanced(Qt5OpenGL_DIR)
	mark_as_advanced(Qt5Widgets_DIR)	

	get_filename_component(Qt5_HOME "${Qt5_DIR}/../../.." REALPATH)

	if (EXISTS "${Qt5_HOME}/plugins")
		set(Qt5_PLUGIN_PATH "${Qt5_HOME}/plugins")
	elseif (EXISTS "${Qt5_HOME}/lib/qt5/plugin")
		set(Qt5_PLUGIN_PATH "${Qt5_HOME}/lib/qt5/plugin")
	else()
		MESSAGE(WARNING "cannot find Qt5 plugins")
		ForceUnset(Qt5_PLUGIN_PATH)
	endif()
	
	if (WIN32)
		string(REPLACE "\\" "/" Qt5_DIR         "${Qt5_DIR}")
		string(REPLACE "\\" "/" Qt5_HOME        "${Qt5_HOME}")
		string(REPLACE "\\" "/" Qt5_PLUGIN_PATH "${Qt5_PLUGIN_PATH}")
	endif()	

	MESSAGE(STATUS "Qt5_DIR        ${Qt5_DIR}")
	MESSAGE(STATUS "Qt5_HOME       ${Qt5_HOME}")
	MESSAGE(STATUS "Qt5_PLUGIN_PATH ${Qt5_PLUGIN_PATH}")

endif()

if (WIN32)
	set(SCRIPT_EXT ".bat")
elseif (APPLE)	
	set(SCRIPT_EXT ".command")
else()
	set(SCRIPT_EXT ".sh")
endif()

if (CMAKE_CONFIGURATION_TYPES)
	set(InstallDir ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus)
else()
	set(InstallDir ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus)
endif()

if (VISUS_PYTHON)
	file(GENERATE   OUTPUT ${InstallDir}/PYTHON_VERSION CONTENT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
	file(GENERATE   OUTPUT ${InstallDir}/Deploy.py      INPUT   ${CMAKE_CURRENT_SOURCE_DIR}/CMake/Deploy.py)
	file(GENERATE   OUTPUT ${InstallDir}/__init__.py    INPUT   ${CMAKE_CURRENT_SOURCE_DIR}/CMake/__init__.py)
	file(GENERATE   OUTPUT ${InstallDir}/__main__.py    INPUT   ${CMAKE_CURRENT_SOURCE_DIR}/CMake/__main__.py)
	file(GENERATE   OUTPUT ${InstallDir}/setup.py       INPUT   ${CMAKE_CURRENT_SOURCE_DIR}/CMake/setup.py)
endif()

if (VISUS_GUI)
	file(GENERATE OUTPUT ${InstallDir}/QT_VERSION       CONTENT "${Qt5Core_VERSION_STRING}")
endif()	

# create scripts
GenerateScript("${CMAKE_CURRENT_SOURCE_DIR}/CMake/script" ${InstallDir}/visus       ${InstallDir}/bin/visus)

if (VISUS_GUI)
	GenerateScript(${CMAKE_CURRENT_SOURCE_DIR}/CMake/script ${InstallDir}/visusviewer ${InstallDir}/bin/visusviewer)
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# install
if (1)

	# avoid problem about permission denied
	#if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	#	set (CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install " CACHE PATH "default install path" FORCE )
	#endif()
	
	install(FILES           LICENSE                                       DESTINATION ${InstallDir}/)
	install(FILES           README.md                                     DESTINATION ${InstallDir}/)
	install(FILES           CMake/OpenVisusConfig.cmake                   DESTINATION ${InstallDir}/)
	install(DIRECTORY       Copyrights                                    DESTINATION ${InstallDir}/)
	install(DIRECTORY       Samples                                       DESTINATION ${InstallDir}/)
	install(DIRECTORY       CMake                                         DESTINATION ${InstallDir}/)
	install(DIRECTORY       datasets                                      DESTINATION ${InstallDir}/)
	install(FILES           datasets/visus.config                         DESTINATION ${InstallDir}/)

	# copy needed windows *.dll
	if (WIN32)
		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ${InstallDir}/bin)
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${InstallDir}/bin)	
	endif()


	# ____________________________________________
	macro(CMakePostInstall)
	
		# fix the problem with <CONFIG> inside
		string(REPLACE "$<CONFIG>" "\${CMAKE_INSTALL_CONFIG_NAME}" OpenVisus_INSTALL_DIR  "${InstallDir}")

		# copy Qt5 plugins
		if (VISUS_GUI AND Qt5_PLUGIN_PATH)
			InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/iconengines    ${InstallDir}/bin/Qt/plugins/)
			InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/platforms      ${InstallDir}/bin/Qt/plugins/)
			InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/printsupport   ${InstallDir}/bin/Qt/plugins/)
			InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/styles         ${InstallDir}/bin/Qt/plugins/)	
		endif()

		if (WIN32)

			# copy Qt5 libraries
			if (VISUS_GUI)
				install(CODE "
					execute_process(COMMAND ${Qt5_HOME}/bin/windeployqt.exe bin/visusviewer.exe
						--libdir    ./bin
						--plugindir ./bin/Qt/plugins
						--no-translations
						WORKING_DIRECTORY ${OpenVisus_INSTALL_DIR})")
			endif()

		else()

			# macdeployqt does not allow to choose the 'layout' of the app
			if (PYTHON_EXECUTABLE)
				install(CODE "
					execute_process(COMMAND ${PYTHON_EXECUTABLE} ${OpenVisus_INSTALL_DIR}/Deploy.py POSTINSTALL RESULT_VARIABLE retcode)
					if(NOT \"\${retcode}\" STREQUAL \"0\")
						message(FATAL_ERROR \"Fatal error executing make self contained\")
					endif()")
			else()
				message(WARNING "I don't have a way to create self contained binaries. Path will be absolute")
			endif()

		endif()

	endmacro()

	add_subdirectory(CMake/PostInstall)

endif()

# //////////////////////////////////////////////////////////////
# *** dist (to call after INSTALL) ***
if (VISUS_PYTHON)
	add_custom_target(DIST COMMAND ${PYTHON_EXECUTABLE} -u \"${InstallDir}/Deploy.py\" DIST)
	set_property(TARGET DIST PROPERTY FOLDER CMakePredefinedTargets)
endif()


# //////////////////////////////////////////////////////////////
# *** CONFIGURE (to call after INSTALL, create scripts and forse use of PyQt) ***
if (VISUS_PYTHON)
	add_custom_target(CONFIGURE COMMAND ${PYTHON_EXECUTABLE} -u \"${InstallDir}/Deploy.py\" CONFIGURE)
	set_property(TARGET CONFIGURE PROPERTY FOLDER CMakePredefinedTargets)
endif()


# //////////////////////////////////////////////////////////////
# *** RUN_TESTS (to call after INSTALL) ***
if (VISUS_PYTHON)

	enable_testing()

	macro(AddOpenVisusTest Name)
		add_test(NAME ${Name} ${ARGN})
		set_property(TEST ${Name} APPEND PROPERTY ENVIRONMENT "CTEST_OUTPUT_ON_FAILURE=1")
		set_property(TEST ${Name} APPEND PROPERTY ENVIRONMENT "PYTHONPATH=${InstallDir}/..")
	endmacro()
	
	AddOpenVisusTest(VisusTestPyDataflow   COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/Dataflow.py"        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusTestPyArray      COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/Array.py"           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusTestPyIdx        COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/Idx.py"             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusTestPyXIdx       COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/XIdx.py"            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	
	AddOpenVisusTest(VisusDataConversion1 COMMAND "${PYTHON_EXECUTABLE}"     "Samples/python/DataConversion1.py" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusDataConversion2 COMMAND "${PYTHON_EXECUTABLE}"     "Samples/python/DataConversion2.py" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	# disabled: takes too much time, in case you modify some core IDX class you can run it manually
	# AddOpenVisusTest(NAME VisusTestIdx COMMAND "$<TARGET_FILE:visus>"     "." --test-idx --max-seconds 300     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	
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
			COMMAND "${CMAKE_COMMAND}" 
				"${CMAKE_GENERATOR_ARGUMENT}"  
				-H"${SourceDir}/" 
				-B"${BinaryDir}/" 
				-DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} 
				-DPYTHON_LIBRARY=${PYTHON_LIBRARY}
				-DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIRS}
				-DQt5_DIR="${Qt5_DIR}" 
				-DOpenVisus_DIR=${InstallDir}
			COMMAND "${CMAKE_COMMAND}"  --build "${BinaryDir}/"  --config ${CMAKE_BUILD_TYPE})

		set_target_properties(${Name} PROPERTIES FOLDER CMakeTargets/)

	endmacro()

	if (VISUS_IDX)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query)
	endif()

	if (VISUS_GUI)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d)
	endif()

endif()


