
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
SetupCommonCMake()

if (NOT DISABLE_OPENMP)
	FindOpenMP(0)
endif()

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
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)
else()
  string(REPLACE "\\" "/" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" CACHE PATH "" FORCE)
endif()


MESSAGE(STATUS "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")

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


MESSAGE(STATUS "VISUS_INTERNAL_ZLIB       ${VISUS_INTERNAL_ZLIB}")
MESSAGE(STATUS "VISUS_INTERNAL_LZ4        ${VISUS_INTERNAL_LZ4}")
MESSAGE(STATUS "VISUS_INTERNAL_TINYXML    ${VISUS_INTERNAL_TINYXML}")
MESSAGE(STATUS "VISUS_INTERNAL_FREEIMAGE  ${VISUS_INTERNAL_FREEIMAGE}")
MESSAGE(STATUS "VISUS_INTERNAL_OPENSSL    ${VISUS_INTERNAL_OPENSSL}")
MESSAGE(STATUS "VISUS_INTERNAL_CURL       ${VISUS_INTERNAL_CURL}")

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


if (WIN32 AND DEFINED PYTHONHOME)

	string(REPLACE "\\" "/" PYTHONHOME "${PYTHONHOME}")
	add_subdirectory(${PYTHONHOME})
	find_package(NumPy REQUIRED)

else()

	if (NOT DEFINED PYTHON_VERSION)
		set(PYTHON_VERSION 3)
	endif()

	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)
	find_package(NumPy QUIET) 
	
	message(STATUS "PYTHON_EXECUTABLE   ${PYTHON_EXECUTABLE}")
	message(STATUS "PYTHON_LIBRARY      ${PYTHON_LIBRARY}")
	message(STATUS "PYTHON_INCLUDE_DIR  ${PYTHON_INCLUDE_DIR}")
	
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
# install
if (True)

	install(FILES        LICENSE                        DESTINATION .)
	install(FILES        README.md                      DESTINATION .)
	install(FILES        CMake/__init__.py              DESTINATION .)
	install(FILES        CMake/OpenVisus.py             DESTINATION .)
	install(FILES        CMake/setup.py                 DESTINATION .)
	install(FILES        CMake/OpenVisusConfig.cmake    DESTINATION .)
	install(DIRECTORY    Copyrights                     DESTINATION .)
	install(DIRECTORY    Samples                        DESTINATION .)
	
	install(FILES        datasets/visus.config          DESTINATION .)
	install(DIRECTORY    datasets/cat                   DESTINATION datasets)
	install(DIRECTORY    datasets/midx                  DESTINATION datasets)
	
	install(DIRECTORY    Libs/Kernel/include/Visus      DESTINATION include/Kernel/)
	install(DIRECTORY    Libs/Dataflow/include/Visus    DESTINATION include/Dataflow/)
	install(DIRECTORY    Libs/Db/include/Visus          DESTINATION include/Db/)
	install(DIRECTORY    Libs/Idx/include/Visus         DESTINATION include/Idx/)
	install(DIRECTORY    Libs/Nodes/include/Visus       DESTINATION include/Nodes)
	
	# swig generated files 
	InstallBuildFiles(*.py ./)
	
	if (WIN32)
	
		InstallBuildFiles(*.dll bin)
		
		Win32CreateBatchFile(${CMAKE_BINARY_DIR}/visus.bat       "bin\\\\visus.exe"       "numpy")
		Win32CreateBatchFile(${CMAKE_BINARY_DIR}/visusviewer.bat "bin\\\\visusviewer.exe" "numpy PyQt5")
		
		install(FILES ${CMAKE_BINARY_DIR}/visus.bat       DESTINATION .)
		install(FILES ${CMAKE_BINARY_DIR}/visusviewer.bat DESTINATION .)
		
		if (DEFINED PYTHONHOME)
			install(DIRECTORY ${PYTHONHOME}/ DESTINATION win32/python)
		endif()		
		
		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	

		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin COMPONENT Libraries)		
		
	endif()

	if (VISUS_GUI)
		install(DIRECTORY Libs/Gui/include/Visus         DESTINATION include/Gui)
		install(DIRECTORY Libs/GuiNodes/include/Visus    DESTINATION include/GuiNodes)
		install(DIRECTORY Libs/AppKit/include/Visus      DESTINATION include/AppKit)
	endif()
	
	
endif()

# /////////////////////////////////////////////////////////////////////
# deploy target
if (True)

	add_custom_target(deploy 
		"${CMAKE_COMMAND}" -E echo "Running deploy step..."
		COMMAND
		"${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/deploy.py" --qt-directory "${Qt5_DIR}"  
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}"
	)
	
	if (WIN32)
		set_target_properties(deploy PROPERTIES FOLDER "CMakePredefinedTargets/")
	endif()
	
endif()

# /////////////////////////////////////////////////////////////////////
# dist target
if (True)
	
	set(PYTHON_TAG cp${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
	
	if (WIN32)
		set(PYTHON_SDIST_FORMAT zip)
	else()
		set(PYTHON_SDIST_FORMAT gztar)
	endif()
	
	if (WIN32)
	
		set(PYTHON_PLAT_NAME win_amd64)
		
	elseif (APPLE)
	
		string(SUBSTRING ${APPLE_OSX_VERSION} 0 2 __major__)
		string(SUBSTRING ${APPLE_OSX_VERSION} 3 2 __minor__)
		set(PYTHON_PLAT_NAME macosx_${__major__}_${__minor__}_x86_64)
		
	else()
	
		execute_process(COMMAND lsb_release -is OUTPUT_VARIABLE __id__   OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND lsb_release -cs OUTPUT_VARIABLE __name__ OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(PYTHON_PLAT_NAME ${__id__}${__name__})
	
	endif()

	add_custom_target(dist 
		        "${CMAKE_COMMAND}" -E echo "Installing setuptools wheel..."
		COMMAND "${PYTHON_EXECUTABLE}" -m pip install --user --upgrade setuptools wheel
		COMMAND "${CMAKE_COMMAND}" -E echo "Creating bdist_wheel..."
		COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/setup.py" -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=${PYTHON_PLAT_NAME} 
		COMMAND "${CMAKE_COMMAND}" -E echo "Creating sdist..."
		COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/setup.py" -q sdist --formats=${PYTHON_SDIST_FORMAT}
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
	
	if (WIN32)
		set_target_properties(dist PROPERTIES FOLDER "CMakePredefinedTargets/")
	endif()	
	
endif()


# /////////////////////////////////////////////////////////////////////
# pypi
if (DEFINED PYPI_USERNAME AND DEFINED PYPI_PASSWORD)

	if (WIN32)
		set(__home__ $ENV{USERPROFILE})
	else()
		set(__home__ $ENV{HOME}/)
	endif()

	set(__filename__ ${__home__}/.pypirc)

	file(WRITE  ${__filename__} "[distutils]\n")
	file(APPEND ${__filename__} "index-servers=\n")
	file(APPEND ${__filename__} "    pypi\n")
	file(APPEND ${__filename__} "[pypi]\n")
	file(APPEND ${__filename__} "username: ${PYPI_USERNAME}\n")
	file(APPEND ${__filename__} "password: ${PYPI_PASSWORD}\n")
	
	add_custom_target(pypi 
		        "${CMAKE_COMMAND}" -E echo "Running pypi step..."
		COMMAND "${CMAKE_COMMAND}" -E env HOME=${__home__} "${PYTHON_EXECUTABLE}" -m pip install --user --upgrade twine
		COMMAND "${CMAKE_COMMAND}" -E env HOME=${__home__} "${PYTHON_EXECUTABLE}" -m twine upload --skip-existing dist/*.whl 
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
	
	if (WIN32)
		set_target_properties(pypi PROPERTIES FOLDER "CMakePredefinedTargets/")
	endif()	  
  
endif()

               
