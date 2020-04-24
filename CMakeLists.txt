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
	file(GENERATE OUTPUT ${InstallDir}/PYTHON_VERSION CONTENT "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
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

	InstallFiles(LICENSE                                      .)
	InstallFiles(README.md                                    .)
	InstallFiles(CMake/OpenVisusConfig.cmake                  .)
	InstallFiles(datasets/visus.config                        .)

	InstallDirectory(Copyrights                               .)
	InstallDirectory(Samples                                  .)
	InstallDirectory(CMake                                    .)
	InstallDirectory(datasets                                 .)
	InstallDirectory(scripts                                  .)

	if (VISUS_PYTHON)
			InstallFiles(CMake/__init__.py         .)
			InstallFiles(CMake/__main__.py         .)
			InstallFiles(CMake/PyDataset.py        .)
			InstallFiles(CMake/PyScriptingNode.py  .)
			InstallFiles(CMake/PyUtils.py          .)
			InstallFiles(CMake/PyViewer.py         .)
			InstallFiles(CMake/setup.py            .)
	endif()

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
		string(REPLACE "$<CONFIG>" "\${CMAKE_INSTALL_CONFIG_NAME}" PostInstallDir  "${InstallDir}")
		
		if (WIN32)

				install(CODE "
					execute_process(
						COMMAND ${Qt5_HOME}/bin/windeployqt.exe bin/visusviewer.exe --libdir ./bin --plugindir ./bin/qt/plugins --no-translations
					WORKING_DIRECTORY ${PostInstallDir})
				")
				
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/iconengines    ./bin/qt/plugins/)
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/platforms      ./bin/qt/plugins/)
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/printsupport   ./bin/qt/plugins/)
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/styles         ./bin/qt/plugins/)	
				
		elif (APPLE)
		
				# macdeployqt does not allow to choose the 'layout' of the app
				install(CODE "
					MESSAGE(STATUS \"Executing post install command ${PYTHON_EXECUTABLE} -u -m OpenVisus post-install\")
					set(ENV{PYTHONPATH} \"${PostInstallDir}/../\")
					execute_process(COMMAND ${PYTHON_EXECUTABLE} -u -m OpenVisus post-install RESULT_VARIABLE __retcode__)
					MESSAGE(STATUS \"  retcode: \${__retcode__} \")
					if(NOT \"\${__retcode__}\" STREQUAL \"0\")
						message(FATAL_ERROR \"Post install failed\")
					endif()
				")
				
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/iconengines    ./bin/qt/plugins/)
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/platforms      ./bin/qt/plugins/)
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/printsupport   ./bin/qt/plugins/)
				InstallDirectoryIfExists(${Qt5_PLUGIN_PATH}/styles         ./bin/qt/plugins/)			
				
		else()
		
			InstallFiles("${Qt5_HOME}/lib/libQt5Core.so*"        ./bin/qt/lib)
			InstallFiles("${Qt5_HOME}/lib/libQt5DBus.so*"        ./bin/qt/lib)
			InstallFiles("${Qt5_HOME}/lib/libQt5Gui.so*"         ./bin/qt/lib)
			InstallFiles("${Qt5_HOME}/lib/libQt5Network.so*"     ./bin/qt/lib)
			InstallFiles("${Qt5_HOME}/lib/libQt5Svg.so*"         ./bin/qt/lib)		
			InstallFiles("${Qt5_HOME}/lib/libQt5Widgets.so*"     ./bin/qt/lib)
			InstallFiles("${Qt5_HOME}/lib/libQt5XcbQpa.so*"      ./bin/qt/lib)
			InstallFiles("${Qt5_HOME}/lib/libQt5OpenGL.so*"      ./bin/qt/lib)	

			# need to copy all, otherwise I will have problems
			InstallDirectory(${Qt5_PLUGIN_PATH}                  ./bin/qt/) 

			# fix rpaths
			install(CODE "
				MESSAGE(STATUS \"Executing ${CMAKE_SOURCE_DIR}/scripts/set_rpath.sh\")
				execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/set_rpath.sh RESULT_VARIABLE __retcode__ WORKING_DIRECTORY \"${PostInstallDir}\")	
				if(NOT \"\${__retcode__}\" STREQUAL \"0\")
						message(FATAL_ERROR \"Post install failed\")
				endif()
			")	
		endif()
	
	endmacro()

	if (VISUS_GUI)
		add_subdirectory(CMake/PostInstall)
	endif()

endif()


# //////////////////////////////////////////////////////////////
# *** use-pyqt (to call after INSTALL, create scripts and forse use of PyQt) ***
if (VISUS_PYTHON)
	add_custom_target(use-pyqt COMMAND ${PYTHON_EXECUTABLE} -u -m OpenVisus use-pyqt)
	set_property(TARGET use-pyqt PROPERTY FOLDER CMakePredefinedTargets)
	set_property(TARGET use-pyqt APPEND PROPERTY ENVIRONMENT "PYTHONPATH=${InstallDir}/..")
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
	AddOpenVisusTest(VisusTestPyDataflow2  COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/Dataflow2.py"       WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusTestPyArray      COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/Array.py"           WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusTestPyIdx        COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/Idx.py"             WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusTestPyXIdx       COMMAND "${PYTHON_EXECUTABLE}"    "Samples/python/XIdx.py"            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	
	AddOpenVisusTest(VisusDataConversion1 COMMAND "${PYTHON_EXECUTABLE}"     "Samples/python/DataConversion1.py" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	AddOpenVisusTest(VisusDataConversion2 COMMAND "${PYTHON_EXECUTABLE}"     "Samples/python/DataConversion2.py" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	# disabled: takes too much time, in case you modify some core IDX class you can run it manually
	# AddOpenVisusTest(NAME VisusTestIdx COMMAND "$<TARGET_FILE:visus>"     "." --test-idx --max-seconds 300     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
	
	macro(AddOpenVisusGuiTest Name Filename)
		add_custom_target(${Name} COMMAND ${CMAKE_COMMAND} -E env "PYTHONPATH=${InstallDir}/.." ${PYTHON_EXECUTABLE} -u \"${Filename}\" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		set_property(TARGET ${Name} PROPERTY FOLDER CMakePredefinedTargets)
	endmacro()
	
	# cannot "Stop in Visual Studio"
	# AddOpenVisusGuiTest(TestViewer1  Samples/python/TestViewer1.py)
	# AddOpenVisusGuiTest(TestViewer2  Samples/python/TestViewer2.py)
	# AddOpenVisusGuiTest(VisibleHuman Samples/python/VisibleHuman.py)
	
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

	if (VISUS_DB)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query)
	endif()

	if (VISUS_GUI)
		AddOpenVisusExternalApp(${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d)
	endif()

endif()


