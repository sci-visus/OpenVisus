CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

# qt-related
if (POLICY CMP0071)
  cmake_policy(SET CMP0071 OLD)
endif()

# see https://cmake.org/cmake/help/git-stage/policy/CMP0086.html
if (POLICY CMP0086)
  cmake_policy(SET CMP0086 OLD)
endif()

if (POLICY CMP0078)
  cmake_policy(SET CMP0078 OLD) #controls names of SWIG targets
endif()

set_property(GLOBAL PROPERTY USE_FOLDERS ON)  
SET(MSVC_INCREMENTAL_DEFAULT OFF)

PROJECT(OpenVisus) 

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (APPLE)
	set(CMAKE_MACOSX_BUNDLE TRUE)
	set(CMAKE_MACOSX_RPATH  TRUE)
endif()

include(FindPackageHandleStandardArgs)
 
if (CMAKE_CONFIGURATION_TYPES)
	set(OutputDir ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus)
else()
	if(NOT DEFINED CMAKE_BUILD_TYPE)
	  set(CMAKE_BUILD_TYPE "Release")
	endif()
	set(OutputDir ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OutputDir}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OutputDir}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${OutputDir}/lib)

find_package(Python COMPONENTS Interpreter Development)	

option(VISUS_GUI "Enable VisusGui" ON)
message(STATUS "VISUS_GUI ${VISUS_GUI}")
if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
	file(GENERATE OUTPUT ${OutputDir}/QT_VERSION CONTENT "${Qt5Core_VERSION_STRING}")
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# install
if (1)

	Install(FILES LICENSE                                    DESTINATION ${OutputDir}/.)
	Install(FILES README.md                                  DESTINATION ${OutputDir}/.)
	Install(FILES CMake/OpenVisusConfig.cmake                DESTINATION ${OutputDir}/.)
	Install(FILES datasets/visus.config                      DESTINATION ${OutputDir}/.)

	Install(DIRECTORY Copyrights                             DESTINATION ${OutputDir}/.)
	Install(DIRECTORY Samples                                DESTINATION ${OutputDir}/.)
	Install(DIRECTORY CMake                                  DESTINATION ${OutputDir}/.)
	Install(DIRECTORY datasets                               DESTINATION ${OutputDir}/.)
	Install(DIRECTORY scripts                                DESTINATION ${OutputDir}/.)

	Install(FILES CMake/__init__.py                          DESTINATION ${OutputDir}/.)
	Install(FILES CMake/__main__.py                          DESTINATION ${OutputDir}/.)
	Install(FILES CMake/PyDataset.py                         DESTINATION ${OutputDir}/.)
	Install(FILES CMake/PyScriptingNode.py                   DESTINATION ${OutputDir}/.)
	Install(FILES CMake/PyUtils.py                           DESTINATION ${OutputDir}/.)
	Install(FILES CMake/PyViewer.py                          DESTINATION ${OutputDir}/.)
	Install(FILES CMake/PyDeploy.py                          DESTINATION ${OutputDir}/.)
	Install(FILES CMake/setup.py                             DESTINATION ${OutputDir}/.)

	# copy needed windows *.dll
	if (WIN32)
		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${OutputDir}/bin")
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${OutputDir}/bin)
	endif()

	# ____________________________________________
	macro(CMakePostInstall)
		get_filename_component(Qt5_HOME "${Qt5_DIR}/../../.." ABSOLUTE)
		string(REPLACE "$<CONFIG>" "\${CMAKE_INSTALL_CONFIG_NAME}" __OutputDir__  "${OutputDir}")
		install(CODE "
			execute_process(
				COMMAND ${PYTHON_EXECUTABLE} -u ${CMAKE_SOURCE_DIR}/CMake/PyDeploy.py --my-deploy \"${Qt5_HOME}\"
					RESULT_VARIABLE __retcode__
					WORKING_DIRECTORY \"${__OutputDir__}\")
			if(NOT \"\${__retcode__}\" STREQUAL \"0\")
				message(FATAL_ERROR \"Post install failed\")
			endif()
		")
	endmacro()

	if (VISUS_GUI)
		add_subdirectory(CMake/PostInstall)
	endif()

endif()

# //////////////////////////////////////////////////////////////
# *** use-pyqt (to call after INSTALL, create scripts and forse use of PyQt) ***
if (1)
	add_custom_target(use-pyqt ${CMAKE_COMMAND} -E env PYTHONPATH=${OutputDir}/.. ${PYTHON_EXECUTABLE} -u -m OpenVisus use-pyqt)
	set_property(TARGET use-pyqt PROPERTY FOLDER CMakePredefinedTargets)
endif()


# ///////////////////////////////////////////////////
# scripts
macro(GenerateScript template_filename script_filename target_filename)

	if (WIN32)
		set(SCRIPT_EXT ".bat")
	elseif (APPLE)	
		set(SCRIPT_EXT ".command")
	else()
		set(SCRIPT_EXT ".sh")
	endif()

	file(READ "${template_filename}${SCRIPT_EXT}" content) 
	
	string(REPLACE "\${PYTHON_EXECUTABLE}" "${PYTHON_EXECUTABLE}" content "${content}")
	string(REPLACE "\${PYTHON_VERSION_MAJOR}" "${PYTHON_VERSION_MAJOR}" content "${content}")
	string(REPLACE "\${PYTHON_VERSION_MINOR}" "${PYTHON_VERSION_MINOR}" content "${content}")

	if (VISUS_GUI)
		string(REPLACE "\${VISUS_GUI}" "1" content "${content}")
	else()
		string(REPLACE "\${VISUS_GUI}" "0" content "${content}")
	endif()
	
	if (WIN32)
		string(REPLACE "\\" "/" target_filename "${target_filename}")
		string(REPLACE "\${TARGET_FILENAME}" "${target_filename}.exe" content  "${content}")
	
	elseif (APPLE)
		get_filename_component(__name_we__ ${target_filename} NAME_WE)
		string(REPLACE "\${TARGET_FILENAME}" "${target_filename}.app/Contents/MacOS/${__name_we__}" content  "${content}")
		
	else()
		string(REPLACE "\${TARGET_FILENAME}" "${target_filename}" content  "${content}")
	endif()
	
	file(GENERATE OUTPUT "${script_filename}${SCRIPT_EXT}" CONTENT "${content}")
	
endmacro()

GenerateScript("${CMAKE_CURRENT_SOURCE_DIR}/CMake/script" ${OutputDir}/visus       bin/visus)

if (VISUS_GUI)
	GenerateScript(${CMAKE_CURRENT_SOURCE_DIR}/CMake/script ${OutputDir}/visusviewer bin/visusviewer)
endif()