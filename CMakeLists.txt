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

PROJECT(ViSUS)

# //////////////////////////////////////////////////////////////
# CMAKE_INSTALL_PREFIX

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "" FORCE)
endif()

if (WIN32)
	string(REPLACE "\\" "/" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif()

MESSAGE(STATUS "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")


include(CMake/VisusMacros.cmake)
SetupCommonCMake()

FindOpenMP()

include(FindPackageHandleStandardArgs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake)

# shared/static linking
option(BUILD_SHARED_LIBS "Build the shared library" TRUE)  

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(VISUS_IS_SUBMODULE 0)
else()
	set(VISUS_IS_SUBMODULE 1)
	set(CMAKE_FOLDER_PREFIX "")
endif()

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


# if using vcpkg on windows use the vcpkg libraries
if (WIN32)
	if (CMAKE_TOOLCHAIN_FILE)
		set(VCPKG 1)
		SetIfNotDefined(VISUS_INTERNAL_DEFAULT 0)
	else()
		set(VCPKG 0)
		SetIfNotDefined(VISUS_INTERNAL_DEFAULT 1)
	endif()	
endif()


SetIfNotDefined(VISUS_INTERNAL_DEFAULT 1)

# ______________________________________________
# *** zlib ***

SetIfNotDefined(VISUS_INTERNAL_ZLIB ${VISUS_INTERNAL_DEFAULT})
if (VISUS_INTERNAL_ZLIB)
	MESSAGE(STATUS "Using internal zlib")
else()
	# i'm assuming brew here
	if (APPLE)
		SetIfNotDefined(ZLIB_INCLUDE_DIR /usr/local/opt/zlib/include)
		SetIfNotDefined(ZLIB_LIBRARY     /usr/local/opt/zlib/lib/libz.dylib)
	endif()
	find_package(ZLIB REQUIRED)
	MESSAGE(STATUS "Using external zlib")
	message(STATUS "ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIRS}")
	message(STATUS "ZLIB_LIBRARIES    ${ZLIB_LIBRARIES}")
endif()

# ______________________________________________
# *** lz4 ***

SetIfNotDefined(VISUS_INTERNAL_LZ4 ${VISUS_INTERNAL_DEFAULT})
if (VISUS_INTERNAL_LZ4)
	MESSAGE(STATUS "Using internal lz4")
else()
	# i'm assuming brew here
	if (APPLE)
		SetIfNotDefined(LZ4_INCLUDE_DIR /usr/local/opt/lz4/include)
		SetIfNotDefined(LZ4_LIBRARY     /usr/local/opt/lz4/lib/liblz4.dylib)
	endif()
	find_package(LZ4 REQUIRED)
	MESSAGE(STATUS "Using external lz4")
	message(STATUS "LZ4_INCLUDE_DIR ${LZ4_INCLUDE_DIR}")
	message(STATUS "LZ4_LIBRARY     ${LZ4_LIBRARY}")
endif()

# ______________________________________________
# *** TinyXML ***

SetIfNotDefined(VISUS_INTERNAL_TINYXML ${VISUS_INTERNAL_DEFAULT})
if (VISUS_INTERNAL_TINYXML)
	MESSAGE(STATUS "Using internal TynyXML")
else()
	# i'm assuming brew here
	if (APPLE)
		SetIfNotDefined(TinyXML_INCLUDE_DIR /usr/local/opt/tinyxml/include)
		SetIfNotDefined(TinyXML_LIBRARY     /usr/local/opt/tinyxml/lib/libtinyxml.dylib)
	endif()
	find_package(TinyXML REQUIRED)
	MESSAGE(STATUS "Using external TynyXML")
	message(STATUS "TinyXML_INCLUDE_DIRS ${TinyXML_INCLUDE_DIRS}")
	message(STATUS "TinyXML_LIBRARIES    ${TinyXML_LIBRARIES}")	
endif()

# ______________________________________________
# *** FreeImage ***

SetIfNotDefined(VISUS_INTERNAL_FREEIMAGE ${VISUS_INTERNAL_DEFAULT})
if (VISUS_INTERNAL_FREEIMAGE)
	MESSAGE(STATUS "Using internal FreeImage")
else()
	# i'm assuming brew here
	if (APPLE)
		SetIfNotDefined(FREEIMAGE_INCLUDE_DIRS /usr/local/opt/freeimage/include)
		SetIfNotDefined(FREEIMAGE_LIBRARIES    /usr/local/opt/freeimage/lib/libfreeimage.dylib)
	endif()
	find_package(FreeImage REQUIRED)
	MESSAGE(STATUS "Using external FreeImage")
	message(STATUS "FREEIMAGE_INCLUDE_DIRS ${FREEIMAGE_INCLUDE_DIRS}")
	message(STATUS "FREEIMAGE_LIBRARIES    ${FREEIMAGE_LIBRARIES}")
endif()

# ______________________________________________
# *** OpenSSL ***

if (WIN32 OR APPLE)
	SetIfNotDefined(VISUS_INTERNAL_OPENSSL ${VISUS_INTERNAL_DEFAULT})
else()
	SetIfNotDefined(VISUS_INTERNAL_OPENSSL 0) # I have problem on linux mixing python openssl and mine, so I'm using system openssl
endif() 

if (VISUS_INTERNAL_OPENSSL)
	MESSAGE(STATUS "Using internal OpenSSL")
else()
	if (UNIX AND (NOT APPLE))
		set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/CMake/FixFindOpenSSL)
	endif()
	# i'm assuming brew here	
	if (APPLE)
		SetIfNotDefined(OPENSSL_INCLUDE_DIR    /usr/local/opt/openssl/include)
		SetIfNotDefined(OPENSSL_CRYPTO_LIBRARY /usr/local/opt/openssl/lib/libcrypto.dylib)
		SetIfNotDefined(OPENSSL_SSL_LIBRARY    /usr/local/opt/openssl/lib/libssl.dylib)	
	endif()
	find_package(OpenSSL REQUIRED)
	MESSAGE(STATUS "Using external OpenSSL")
	message(STATUS "OPENSSL_INCLUDE_DIR    ${OPENSSL_INCLUDE_DIR}")
	message(STATUS "OPENSSL_CRYPTO_LIBRARY ${OPENSSL_CRYPTO_LIBRARY}")
	message(STATUS "OPENSSL_SSL_LIBRARY    ${OPENSSL_SSL_LIBRARY}")
endif()

# ______________________________________________
# *** curl ***

SetIfNotDefined(VISUS_INTERNAL_CURL ${VISUS_INTERNAL_DEFAULT})

if (VISUS_INTERNAL_CURL)
	MESSAGE(STATUS "Using internal Curl")
else()
	# i'm assuming brew here
	if (APPLE)
		SetIfNotDefined(CURL_INCLUDE_DIR /usr/local/opt/curl/include)
		SetIfNotDefined(CURL_LIBRARY     /usr/local/opt/curl/lib/libcurl.dylib)
	endif()
	find_package(CURL REQUIRED)
	# see https://github.com/Microsoft/vcpkg/issues/1909
 	if (VCPKG)
		list(LENGTH CURL_LIBRARY CURL_LIBRARY_LENGTH)
		if (CURL_LIBRARY_LENGTH EQUAL 1)
			set(CURL_LIBRARY_DEBUG_LIB   ${CURL_LIBRARY})
			set(CURL_LIBRARY_RELEASE_LIB ${CURL_LIBRARY_DEBUG_LIB}/../../../lib/libcurl.lib)
			get_filename_component(CURL_LIBRARY_RELEASE_LIB ${CURL_LIBRARY_RELEASE_LIB} REALPATH)
			ForceUnset(CURL_LIBRARY)
			ForceUnset(CURL_LIBRARIES)
			set(CURL_LIBRARY "debug;${CURL_LIBRARY_DEBUG_LIB};optimized;${CURL_LIBRARY_RELEASE_LIB}")
			set(CURL_LIBRARIES ${CURL_LIBRARY})
		endif()
  endif()	
  message(STATUS "Using external Curl")
  message(STATUS "CURL_INCLUDE_DIR ${CURL_INCLUDE_DIR}")
	message(STATUS "CURL_LIBRARIES   ${CURL_LIBRARIES}")
endif()

# ______________________________________________
# *** python

FindPythonLibrary()

add_subdirectory(InternalLibs)
add_subdirectory(Libs)
add_subdirectory(Executable)

# //////////////////////////////////////////////////////////////
# *** testing and external apps ***
if (1)

	# testing
	enable_testing()

  AddCTest(VisusTestPyDataflow ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Dataflow.py)
  AddCTest(VisusTestPyArray    ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Array.py)
  AddCTest(VisusTestPyIdx      ${PYTHON_EXECUTABLE}  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/Samples/python/Idx.py)
	
  # disabled: takes too much time, in case you modify some core IDX class you can run it manually
  if (0)
  	AddCTest(VisusTestIdx      $<TARGET_FILE:visus> ${CMAKE_CURRENT_SOURCE_DIR}  --test-idx --max-seconds 300)
  endif()
	
	# external app
	AddExternalApp(simple_query ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_query ${CMAKE_BINARY_DIR}/Samples/simple_query)

	if (VISUS_GUI)
		AddExternalApp(simple_viewer2d ${CMAKE_CURRENT_SOURCE_DIR}/Samples/simple_viewer2d ${CMAKE_BINARY_DIR}/Samples/simple_viewer2d)
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
	
	set (__filename__ )
	file(WRITE    "${CMAKE_BINARY_DIR}/PYTHON_VERSION" "${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
	install(FILES "${CMAKE_BINARY_DIR}/PYTHON_VERSION"  DESTINATION .)
	
	# swig generated files 
	InstallBuildFiles(*.py ./)
	
	createScript(visus "")
	
	if (WIN32)
	
		InstallBuildFiles(*.dll bin)
	
		if (OpenMP_FOUND)
			set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
		endif()	
		
		# self-contained python
		get_filename_component(__python_dir__ ${PYTHON_EXECUTABLE} DIRECTORY)
		install(DIRECTORY ${__python_dir__}/ DESTINATION win32/python 
		  PATTERN "*.pyc" EXCLUDE 
		)

		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin COMPONENT Libraries)	
		
	endif()

	if (VISUS_GUI)
	
		install(DIRECTORY Libs/Gui/include/Visus         DESTINATION include/Gui)
		install(DIRECTORY Libs/GuiNodes/include/Visus    DESTINATION include/GuiNodes)
		install(DIRECTORY Libs/AppKit/include/Visus      DESTINATION include/AppKit)
		
		# install Qt plugins
		if (EXISTS "${Qt5_DIR}/../../../plugins")
			set(qt5_PLUGIN_DIR "${Qt5_DIR}/../../../plugins")
			
		elseif (EXISTS "${Qt5_DIR}/../../qt5/plugins")
			set(qt5_PLUGIN_DIR "${Qt5_DIR}/../../qt5/plugins")
			
		else()
			MESSAGE(ERROR "Cannot find qt plugins")
		endif()

		macro(InstallQtPlugin name)
			if (EXISTS "${qt5_PLUGIN_DIR}/${name}")
				Install(DIRECTORY "${qt5_PLUGIN_DIR}/${name}" DESTINATION bin/plugins)
			endif()
		endmacro()
		
		InstallQtPlugin(iconengines)
		InstallQtPlugin(imageformats)
		InstallQtPlugin(platforms)
		InstallQtPlugin(printsupport)
		InstallQtPlugin(styles)
		
      createGuiScript(visusviewer "")
    
	endif()

	
endif()



# /////////////////////////////////////////////////////////////////////
# note: alternative would be to use cmake FIXUP_BUNDLE... but it seems broken on Linux
# and also would have problmes with run-time linking (i.e. Qt5 plugins)
# deploy target
if (True)

	if (WIN32)
	
		add_custom_target(deploy 
			        "${CMAKE_COMMAND}" -E echo "Running deploy step..."
		   COMMAND	"${Qt5_DIR}\\..\\..\\..\\bin\\windeployqt" "bin\\visusviewer.exe" --libdir bin --plugindir bin\\plugins --no-translations
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
			
		set_target_properties(deploy PROPERTIES FOLDER "CMakePredefinedTargets/")
	
	else()

		add_custom_target(deploy 
		  "${CMAKE_COMMAND}" -E echo "Running deploy step..."
			COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/deploy.py" --fix-deps 
			COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/deploy.py" --show-deps 
			WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")

	endif()
	
endif()

# /////////////////////////////////////////////////////////////////////
# sdist target
if (True)
	
	if (WIN32)
		set(PYTHON_SDIST_FORMAT zip)
	else()
		set(PYTHON_SDIST_FORMAT gztar)
	endif()
	
	add_custom_target(sdist 
		        "${CMAKE_COMMAND}" -E echo "Installing setuptools..."
		COMMAND "${PYTHON_EXECUTABLE}" -m pip install --user --upgrade setuptools 
		COMMAND "${CMAKE_COMMAND}" -E echo "Running sdist target..."
		COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/setup.py" -q sdist --formats=${PYTHON_SDIST_FORMAT}
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
	
	if (WIN32)
		set_target_properties(sdist PROPERTIES FOLDER "CMakePredefinedTargets/")
	endif()	
	
endif()


# /////////////////////////////////////////////////////////////////////
# bdist_wheel target
if (True)

	set(PYTHON_TAG cp${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})

	if (WIN32)
	
		set(PYTHON_PLAT_NAME win_amd64)
		
	elseif (APPLE)
	
		string(SUBSTRING ${APPLE_OSX_VERSION} 0 2 __major__)
		string(SUBSTRING ${APPLE_OSX_VERSION} 3 2 __minor__)
		set(PYTHON_PLAT_NAME macosx_${__major__}_${__minor__}_x86_64)
		
	else()
	
		execute_process(COMMAND lsb_release -is OUTPUT_VARIABLE __id__   OUTPUT_STRIP_TRAILING_WHITESPACE)
		execute_process(COMMAND lsb_release -cs OUTPUT_VARIABLE __name__ OUTPUT_STRIP_TRAILING_WHITESPACE)
		set(PYTHON_PLAT_NAME "${__id__}.${__name__}")
	
	endif()

	add_custom_target(bdist_wheel 
		        "${CMAKE_COMMAND}" -E echo "Installing setuptools wheel..."
		COMMAND "${PYTHON_EXECUTABLE}" -m pip install --user --upgrade setuptools wheel
		COMMAND "${CMAKE_COMMAND}" -E echo "Running bdist_wheel target..."
		COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/setup.py" -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=${PYTHON_PLAT_NAME} 
		WORKING_DIRECTORY "${CMAKE_INSTALL_PREFIX}")
	
	if (WIN32)
		set_target_properties(bdist_wheel PROPERTIES FOLDER "CMakePredefinedTargets/")
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

               
