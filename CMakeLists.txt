
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

if(POLICY CMP0068)
  cmake_policy(SET CMP0068 NEW)
endif()

#if (POLICY CMP0040)
#	cmake_policy(SET CMP0040 OLD)
#endif()

PROJECT(ViSUS)

include(CMake/VisusMacros.cmake)
SetupCMake()

include(FindPackageHandleStandardArgs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

# shared/static linking
option(BUILD_SHARED_LIBS "Build the shared library" TRUE)  

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(VISUS_IS_SUBMODULE 0)
	set(CMAKE_FOLDER_PREFIX "")
else()
	set(VISUS_IS_SUBMODULE 1)
	SET(CMAKE_FOLDER_PREFIX OpenVisus/)
endif()


# enable/disable install
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)

# enable/disable gui-stuff (i.e. Qt dependent)
option(VISUS_GUI "Enable gui" TRUE)  

IF (VISUS_GUI)

	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED)
	
	mark_as_advanced(Qt5Core_DIR)
	mark_as_advanced(Qt5Gui_DIR)
	mark_as_advanced(Qt5OpenGL_DIR)
	mark_as_advanced(Qt5Widgets_DIR)

	if (WIN32)
		string(REPLACE "\\" "/" Qt5_DIR "${Qt5_DIR}")
	endif()
	
	if (WIN32)
		set(QT5_BINARY_DIR "${Qt5_DIR}/../../../bin")
		find_program(DEPLOYQT NAMES windeployqt PATHS "${QT5_BINARY_DIR}")
		
		if(NOT DEPLOYQT)
			message(FATAL_ERROR "Could not find windeployqt")
		endif()
		
		mark_as_advanced(DEPLOYQT)
	endif()
	
	if (APPLE)
		set(QT5_BINARY_DIR "${Qt5_DIR}/../../../bin")
	  find_program(DEPLOYQT NAMES macdeployqt PATHS "${QT5_BINARY_DIR}")
	  
		if(NOT DEPLOYQT)
			message(FATAL_ERROR "Could not find macdeployqt")
		endif()	  
		
		mark_as_advanced(DEPLOYQT)
	  
	endif()
	
endif()

if (NOT DEFINED VISUS_INTERNAL_ZLIB)
	set(VISUS_INTERNAL_ZLIB 1)
endif()

if (NOT DEFINED VISUS_INTERNAL_LZ4)
	set(VISUS_INTERNAL_LZ4 1)
endif()

if (NOT DEFINED VISUS_INTERNAL_TINYXML)
	set(VISUS_INTERNAL_TINYXML 1)
endif()

if (NOT DEFINED VISUS_INTERNAL_FREEIMAGE)
	set(VISUS_INTERNAL_FREEIMAGE 1)
endif()

if (NOT DEFINED VISUS_INTERNAL_OPENSSL)
	if (WIN32 OR APPLE)
		set(VISUS_INTERNAL_OPENSSL 1)
	else()
		set(VISUS_INTERNAL_OPENSSL 0) # seem to have problem on linux mixing python openssl and mine
	endif() 
endif()

if (NOT DEFINED VISUS_INTERNAL_CURL)
	set(VISUS_INTERNAL_CURL 1)
endif()

if (NOT DEFINED VISUS_INTERNAL_PYTHON)
	set(VISUS_INTERNAL_PYTHON 0)
endif()

MESSAGE(STATUS "VISUS_INTERNAL_ZLIB       ${VISUS_INTERNAL_ZLIB}")
MESSAGE(STATUS "VISUS_INTERNAL_LZ4        ${VISUS_INTERNAL_LZ4}")
MESSAGE(STATUS "VISUS_INTERNAL_TINYXML    ${VISUS_INTERNAL_TINYXML}")
MESSAGE(STATUS "VISUS_INTERNAL_FREEIMAGE  ${VISUS_INTERNAL_FREEIMAGE}")
MESSAGE(STATUS "VISUS_INTERNAL_OPENSSL    ${VISUS_INTERNAL_OPENSSL}")
MESSAGE(STATUS "VISUS_INTERNAL_CURL       ${VISUS_INTERNAL_CURL}")
MESSAGE(STATUS "VISUS_INTERNAL_PYTHON     ${VISUS_INTERNAL_PYTHON}")

if (NOT VISUS_INTERNAL_ZLIB)
	find_package(ZLIB REQUIRED)
endif()

if (NOT VISUS_INTERNAL_LZ4)
	find_package(LZ4 REQUIRED)
endif()

if (NOT VISUS_INTERNAL_TINYXML)
	find_package(TinyXML REQUIRED)
endif()

if (NOT VISUS_INTERNAL_FREEIMAGE)
	find_package(FreeImage REQUIRED)
endif()

if (NOT VISUS_INTERNAL_OPENSSL)
	if (UNIX)
		set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FixFindOpenSSL)
	endif()
	find_package(OpenSSL)
endif()

if (NOT VISUS_INTERNAL_CURL)
	find_package(CURL REQUIRED)
endif()

# python
if (NOT DEFINED PYTHON_VERSION)
	set(PYTHON_VERSION 3)
endif()

if (NOT VISUS_INTERNAL_PYTHON)
	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	find_package(PythonLibs  ${PYTHON_VERSION} REQUIRED)
	find_package(NumPy QUIET) 
	message("PYTHON_EXECUTABLE   ${PYTHON_EXECUTABLE}")
	message("PYTHON_LIBRARY      ${PYTHON_LIBRARY}")
	message("PYTHON_INCLUDE_DIR  ${PYTHON_INCLUDE_DIR}")
endif()

add_subdirectory(InternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# *** samples and external apps ***
if (NOT VISUS_IS_SUBMODULE)

	# samples
	add_subdirectory(Samples)

	# testing
	enable_testing()

  AddCTest(VisusTestPyDataflow ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Dataflow.py)
  AddCTest(VisusTestPyArray    ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Array.py)
  AddCTest(VisusTestPyIdx      ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py)
	
  # disabled: takes too much time, in case you modify some core IDX class you can run it manually
  # AddCTest(VisusTestIdx      $<TARGET_FILE:visus> ${CMAKE_CURRENT_SOURCE_DIR}  --test-idx --max-seconds 300)
	
	# external app
	AddExternalApp(external_simple_query ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query ${CMAKE_BINARY_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddExternalApp(external_simple_viewer2d ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d ${CMAKE_BINARY_DIR}/Samples/simple_viewer2d)
	endif()

endif()

# //////////////////////////////////////////////////////////////
# *** install ***
if (True)

	install(FILES        LICENSE                        DESTINATION .)
	install(FILES        README.md                      DESTINATION .)
	install(FILES        CMake/__init__.py              DESTINATION .)
	install(FILES        CMake/OpenVisus.py             DESTINATION .)
	install(FILES        CMake/setup.py                 DESTINATION .)
	install(FILES        CMake/OpenVisusConfig.cmake    DESTINATION .)
	install(DIRECTORY    Copyrights                     DESTINATION .)
	install(DIRECTORY    Samples                        DESTINATION .)
	install(DIRECTORY    datasets                       DESTINATION .)
	
	install(DIRECTORY    Libs/Kernel/include/Visus      DESTINATION include/Kernel/)
	install(DIRECTORY    Libs/Dataflow/include/Visus    DESTINATION include/Dataflow/)
	install(DIRECTORY    Libs/Db/include/Visus          DESTINATION include/Db/)
	install(DIRECTORY    Libs/Idx/include/Visus         DESTINATION include/Idx/)
	install(DIRECTORY    Libs/Nodes/include/Visus       DESTINATION include/Nodes)
	
	# swig generated files 
	InstallBuildFiles(*.py ./)
	
	if (WIN32)
	  InstallBuildFiles(*.dll ./)
	endif()
	
	if (VISUS_GUI)
		install(DIRECTORY Libs/Gui/include/Visus         DESTINATION include/Gui)
		install(DIRECTORY Libs/GuiNodes/include/Visus    DESTINATION include/GuiNodes)
		install(DIRECTORY Libs/AppKit/include/Visus      DESTINATION include/AppKit)
	endif()
	
	
	# ///////////////////////////////////////////////////
	macro(PostInstallStep)
	
		if (VISUS_GUI)
		
			if (WIN32)
				install(CODE "
				  message(STATUS 'Deploying qt...')
					if(\${BUILD_TYPE} STREQUAL \"Debug\")
						execute_process(COMMAND ${DEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/debug/visusviewer.exe\" WORKING_DIRECTORY ${QT5_BINARY_DIR})
					else()
						execute_process(COMMAND ${DEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/visusviewer.exe\"       WORKING_DIRECTORY ${QT5_BINARY_DIR})
					endif()
				")	
			endif()
			
			if (APPLE)
				install(CODE "
				  message(STATUS 'Deploying qt...')
					if(\${BUILD_TYPE} STREQUAL \"Debug\")
						execute_process(COMMAND ${DEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/debug/visusviewer.app\" WORKING_DIRECTORY ${QT5_BINARY_DIR})
					else()
						execute_process(COMMAND ${DEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/visusviewer.app\"       WORKING_DIRECTORY ${QT5_BINARY_DIR})
					endif()
					execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../CMake/postinstall.apple.py WORKING_DIRECTORY \${CMAKE_INSTALL_PREFIX})
				")	
			endif()
		
		endif()
	endmacro()	
	
	
	# Post install step
	add_subdirectory(CMake)
	
endif()
