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
CMAKE_DEPENDENT_OPTION(VISUS_NODES       "Enable VisusNodes"         ON  "VISUS_DATAFLOW;VISUS_DB;"  OFF)
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
message(STATUS "VISUS_NODES       ${VISUS_NODES}")
message(STATUS "VISUS_GUINODES    ${VISUS_GUINODES}")
message(STATUS "VISUS_OSPRAY      ${VISUS_OSPRAY}")
message(STATUS "VISUS_APPKIT      ${VISUS_APPKIT}")

# To activate the slam:
#    git submodule init 
#    git submodule update --init Libs/Slam

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Libs/Slam/CMakeLists.txt)
	option(VISUS_SLAM "Enable slam" ON) 	
	message(STATUS "VISUS_SLAM        ${VISUS_SLAM}")
endif()

if (CMAKE_CONFIGURATION_TYPES)
	set(InstallDir    ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus)
else()
	set(InstallDir    ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus)
endif()

if (VISUS_PYTHON)
	FindPythonLibrary()
	file(GENERATE OUTPUT ${InstallDir}/PYTHON_VERSION_STRING CONTENT "${PYTHON_VERSION_STRING}")
endif()

if (VISUS_GUI)
	FindQtLibrary()
	file(GENERATE OUTPUT ${InstallDir}/QT_VERSION CONTENT "${Qt5Core_VERSION_STRING}")
endif()

if (VISUS_OSPRAY)
	find_package(ospray 2.0 REQUIRED)
	find_package(ospcommon REQUIRED)
endif()


GenerateScript("${CMAKE_CURRENT_SOURCE_DIR}/CMake/script" ${InstallDir}/visus bin/visus)

if (VISUS_GUI)
	GenerateScript(${CMAKE_CURRENT_SOURCE_DIR}/CMake/script ${InstallDir}/visusviewer bin/visusviewer)
endif()

add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# install
if (1)

	InstallFile(LICENSE                                      .)
	InstallFile(README.md                                    .)
	InstallFile(CMake/OpenVisusConfig.cmake                  .)
	InstallFile(datasets/visus.config                        .)

	InstallDirectory(Copyrights                               .)
	InstallDirectory(Samples                                  .)
	InstallDirectory(CMake                                    .)
	InstallDirectory(datasets                                 .)
	InstallDirectory(scripts                                  .)

	if (VISUS_PYTHON)
		InstallFile(CMake/__init__.py         .)
		InstallFile(CMake/__main__.py         .)
		InstallFile(CMake/PyDataset.py        .)
		InstallFile(CMake/PyScriptingNode.py  .)
		InstallFile(CMake/PyUtils.py          .)
		InstallFile(CMake/PyViewer.py         .)
		InstallFile(CMake/PyDeploy.py         .)
		InstallFile(CMake/setup.py            .)
	endif()

	# copy needed windows *.dll
	if (WIN32)
		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${InstallDir}/bin")
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${InstallDir}/bin")
	endif()

	# ____________________________________________
	macro(CMakePostInstall)
	
		# fix the problem with <CONFIG> inside
		string(REPLACE "$<CONFIG>" "\${CMAKE_INSTALL_CONFIG_NAME}" PostInstallDir  "${InstallDir}")
		install(CODE "
			execute_process(
				COMMAND ${PYTHON_EXECUTABLE} -u ${CMAKE_SOURCE_DIR}/CMake/PyDeploy.py --my-deploy \"${Qt5_HOME}\"
					RESULT_VARIABLE __retcode__
					WORKING_DIRECTORY \"${PostInstallDir}\")
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
if (VISUS_PYTHON)
	add_custom_target(use-pyqt ${CMAKE_COMMAND} -E env PYTHONPATH=${InstallDir}/.. ${PYTHON_EXECUTABLE} -u -m OpenVisus use-pyqt)
	set_property(TARGET use-pyqt PROPERTY FOLDER CMakePredefinedTargets)
endif()

