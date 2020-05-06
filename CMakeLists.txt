CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

# qt-related
if(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
endif()

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

# see https://cmake.org/pipermail/cmake/2015-May/060700.html

if (CMAKE_CONFIGURATION_TYPES)
	set(ConfigName    $<CONFIG>)
	set(IntConfigName ${CMAKE_CFG_INTDIR})
else()

	if (NOT DEFINED CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE "Release")
	endif()

	set(ConfigName    ${CMAKE_BUILD_TYPE})
	set(IntConfigName ${CMAKE_BUILD_TYPE})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}/lib)

set (CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE STRING "CMAKE_INSTALL_PREFIX" FORCE)

option(VISUS_NET    "Enable VISUS_NET"    ON)
option(VISUS_IMAGE  "Enable VISUS_IMAGE"  ON)
option(VISUS_PYTHON "Enable VISUS_PYTHON" ON)
option(VISUS_GUI    "Enable VISUS_GUI"    ON)

message(STATUS "VISUS_NET    ${VISUS_NET}")
message(STATUS "VISUS_IMAGE  ${VISUS_IMAGE}")
message(STATUS "VISUS_PYTHON ${VISUS_PYTHON}")
message(STATUS "VISUS_GUI    ${VISUS_GUI}")

if (VISUS_PYTHON)
	find_package(Python COMPONENTS Interpreter Development REQUIRED)

	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})	

	macro(AddSwigLibrary NamePy WrappedLib SwigFile)

		# this is for generated C++ and header files
		set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${IntConfigName}) 

		set(CMAKE_SWIG_OUTDIR ${SWIG_OUTFILE_DIR}/OpenVisus) # this is for *.py generated files
		set (UseSWIG_TARGET_NAME_PREFERENCE STANDARD)
		set(CMAKE_SWIG_FLAGS "")
		#set(CMAKE_SWIG_FLAGS -interface;${NamePy}) # important for good name generation id Debug mode (https://gitlab.kitware.com/cmake/cmake/issues/18771)

		# set_property(SOURCE ${SwigFile} PROPERTY SWIG_MODULE_NAME ${NamePy}) prevents rebuild every time make is called (? scrgiorgio sommented for now)
		set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
		set_source_files_properties(${SwigFile} PROPERTIES SWIG_FLAGS  "${ARGN};-threads;-extranative")

		swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile})
		target_compile_definitions(${NamePy} PRIVATE SWIG_TYPE_TABLE=OpenVisus)

		target_link_libraries(${NamePy} PUBLIC ${WrappedLib})
		set_target_properties(${NamePy} PROPERTIES FOLDER Swig/)

		if (WIN32)
			target_compile_definitions(${NamePy} PRIVATE /W0) # disable warnings
		else()
			set_target_properties(${NamePy} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
		endif()
	
		# swig generated *.py file and *.so must be in the same OpenVisus/ root directoryc otherwise it won't work (since swig auto-generate "from . import _VisusKernelPy") 
		set_target_properties(${NamePy} PROPERTIES 
			LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME} 
			RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}
			ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/swig) 

	endmacro()

endif()

if (VISUS_GUI)
	find_package(Qt5 COMPONENTS Core Widgets Gui OpenGL REQUIRED PATHS ${Qt5_DIR} NO_DEFAULT_PATH)
endif()


add_subdirectory(ExternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# install
if (1)

	set(__qt_version_filename__ ${CMAKE_BINARY_DIR}/${ConfigName}/QT_VERSION)
	file(GENERATE OUTPUT ${__qt_version_filename__} CONTENT "${Qt5Core_VERSION_STRING}")

	INSTALL(FILES LICENSE                        DESTINATION   ${ConfigName}/${PROJECT_NAME}/)
	INSTALL(FILES README.md                      DESTINATION   ${ConfigName}/${PROJECT_NAME}/)
	INSTALL(FILES datasets/visus.config          DESTINATION   ${ConfigName}/${PROJECT_NAME}/)
	INSTALL(FILES ${__qt_version_filename__}     DESTINATION   ${ConfigName}/${PROJECT_NAME}/)
	INSTALL(FILES setup.py                       DESTINATION   ${ConfigName}/${PROJECT_NAME}/)
	INSTALL(FILES deploy.py                      DESTINATION   ${ConfigName}/${PROJECT_NAME}/)

	INSTALL(DIRECTORY Copyrights                 DESTINATION   ${ConfigName}/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)
	INSTALL(DIRECTORY Samples                    DESTINATION   ${ConfigName}/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)
	INSTALL(DIRECTORY CMake                      DESTINATION   ${ConfigName}/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)
	INSTALL(DIRECTORY datasets                   DESTINATION   ${ConfigName}/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)
	INSTALL(DIRECTORY scripts                    DESTINATION   ${ConfigName}/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)

	# copy needed windows *.dll
	if (WIN32)
		set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${ConfigName}/${PROJECT_NAME}/bin")
		include(InstallRequiredSystemLibraries)
		INSTALL(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${ConfigName}/${PROJECT_NAME}/bin/")
	endif()

	# ____________________________________________
	macro(CMakePostInstall)
		if (VISUS_GUI AND VISUS_PYTHON)
			get_filename_component(Qt5_HOME "${Qt5_DIR}/../../.." ABSOLUTE)
			string(REPLACE "\\" "/" __Python_EXECUTABLE__  "${Python_EXECUTABLE}")
			# mportant: I didn't find a way to set PYTHONPATH here inside INSTALL(CODE), so I'm using directly the WORKING_DIRECTORY (without the module)
			INSTALL(CODE "
				message(STATUS \"Executing post install....\")
				execute_process(
					COMMAND ${__Python_EXECUTABLE__} -u deploy.py install-qt5 \"${Qt5_HOME}\" \${CMAKE_INSTALL_CONFIG_NAME}
						RESULT_VARIABLE __retcode__
						WORKING_DIRECTORY \"${CMAKE_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${PROJECT_NAME} \")
				if(NOT \"\${__retcode__}\" STREQUAL \"0\")
					message(FATAL_ERROR \"Post install failed\")
				endif()
			")
		endif()
	endmacro()

	add_subdirectory(CMake/PostInstall)
	
endif()

# //////////////////////////////////////////////////////////////
# *** CONFIGURE (to call after INSTALL, create scripts and forse use of PyQt) ***
if (VISUS_PYTHON)
	add_custom_target(CONFIGURE ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_BINARY_DIR}/${ConfigName}/OpenVisus/.. ${Python_EXECUTABLE} -u -m OpenVisus configure)
	set_property(TARGET CONFIGURE PROPERTY FOLDER CMakePredefinedTargets)
endif()


