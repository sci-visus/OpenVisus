
CMAKE_MINIMUM_REQUIRED(VERSION 2.8) 

include(CMake/VisusMacros.cmake)
SetupCommonCMake()

PROJECT(ViSUS)

FindOpenMP()

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


# if using vcpkg on windows use the vcpkg libraries
if (WIN32)
	if (VCPKG)
		SetIfNotDefined(VISUS_INTERNAL_DEFAULT 0)
	else()
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
  
  MESSAGE(STATUS "Using external Curl")
  message(STATUS "CURL_INCLUDE_DIR ${CURL_INCLUDE_DIR}")
	message(STATUS "CURL_LIBRARIES   ${CURL_LIBRARIES}")
  
endif()

# ______________________________________________
# *** python

# example: git submodule update --init win32/python36
if (WIN32)

	 IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/win32/python36/CMakeLists.txt)
		SetIfNotDefined(VISUS_INTERNAL_PYTHON ${CMAKE_CURRENT_SOURCE_DIR}/win32/python36)

	elseif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/win32/python37/CMakeLists.txt)
		SetIfNotDefined(VISUS_INTERNAL_PYTHON ${CMAKE_CURRENT_SOURCE_DIR}/win32/python37)
		
	endif()
	
endif()


if (VISUS_INTERNAL_PYTHON)

	string(REPLACE "\\" "/" VISUS_INTERNAL_PYTHON "${VISUS_INTERNAL_PYTHON}")
	MESSAGE(STATUS "Using internal python: ${VISUS_INTERNAL_PYTHON}")
	
	add_subdirectory(${VISUS_INTERNAL_PYTHON})
	find_package(NumPy REQUIRED)
else()

	SetIfNotDefined(PYTHON_VERSION 3)

	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)
	find_package(NumPy QUIET) 
	
	MESSAGE(STATUS "Using external python")
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
		
		set (__filename__ "${CMAKE_BINARY_DIR}/visus.bat")
		file(WRITE    "${__filename__}" "cd /d %~dp0\r\n")
		file(APPEND   "${__filename__}" "if EXIST %cd%\\win32\\Python (set PYTHON_DIR=%cd%\\win32\\Python) else (set PYTHON_DIR=%cd%\\..\\..\\..\\) \r\n")
		file(APPEND   "${__filename__}" "%PYTHON_DIR%\\python.exe -m pip install --user --upgrade numpy\r\n")
		file(APPEND   "${__filename__}" "set PATH=%PYTHON_DIR%;%cd%\\bin;%PATH%\r\n")
		file(APPEND   "${__filename__}" "bin\\\\visus.exe %*\r\n")
		install(FILES "${__filename__}"  DESTINATION . )
		
	elseif (APPLE)
	
		set(__filename__ "${CMAKE_BINARY_DIR}/visus.command")
		file(WRITE    "${__filename__}" "#!/bin/bash\n")
		file(APPEND   "${__filename__}" "this_dir=\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\" )\" && pwd)\n")
		file(APPEND   "${__filename__}" "cd \${this_dir}\n")
		file(APPEND   "${__filename__}" "export PATH=\${this_dir}/bin:\$PATH\n")
		file(APPEND   "${__filename__}" "export PYTHONPATH=\${this_dir}:\${this_dir}/bin:\$PYTHONPATH\n")
		file(APPEND   "${__filename__}" "./bin/visus.app/Contents/MacOS/visus\n")
		install(FILES "${__filename__}"  DESTINATION . PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)
	
	else()
	
		set(__filename__ "${CMAKE_BINARY_DIR}/visus.sh")
		file(WRITE    "${__filename__}" "#!/bin/bash\n")
		file(APPEND   "${__filename__}" "this_dir=\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\" )\" && pwd)\n")
		file(APPEND   "${__filename__}" "cd \${this_dir}\n")
		file(APPEND   "${__filename__}" "export PATH=\${this_dir}/bin:\$PATH\n")
		file(APPEND   "${__filename__}" "export PYTHONPATH=\${this_dir}:\${this_dir}/bin:\$PYTHONPATH\n")
		file(APPEND   "${__filename__}" "./bin/visus\n")
		install(FILES "${__filename__}"  DESTINATION . PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)	
	
	endif()
	
	if (WIN32)
	
		InstallBuildFiles(*.dll bin)
	
		if (DEFINED VISUS_INTERNAL_PYTHON)
			install(DIRECTORY ${VISUS_INTERNAL_PYTHON}/ DESTINATION win32/python)
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
		
		# install Qt plugins
		if (EXISTS "${Qt5_DIR}/../../../plugins")
			set(qt5_PLUGIN_DIR "${Qt5_DIR}/../../../plugins")
			
		elseif (EXISTS "${Qt5_DIR}/../../qt5/plugins")
			set(qt5_PLUGIN_DIR "${Qt5_DIR}/../../qt5/plugins")
			
		else()
			MESSAGE(ERROR "Cannot find qt plugins")
		endif()
		
		if (NOT WIN32)
			install(DIRECTORY "${qt5_PLUGIN_DIR}/iconengines"   DESTINATION bin/plugins)
			install(DIRECTORY "${qt5_PLUGIN_DIR}/imageformats"  DESTINATION bin/plugins)
			install(DIRECTORY "${qt5_PLUGIN_DIR}/platforms"     DESTINATION bin/plugins)
			install(DIRECTORY "${qt5_PLUGIN_DIR}/printsupport"  DESTINATION bin/plugins)
			install(DIRECTORY "${qt5_PLUGIN_DIR}/styles"        DESTINATION bin/plugins)
		endif()
		
		if (WIN32)
				
			set(__filename__ "${CMAKE_BINARY_DIR}/visusviewer.bat")
			file(WRITE    "${__filename__}" "cd /d %~dp0\r\n")
			file(APPEND   "${__filename__}" "if EXIST %cd%\\win32\\Python (set PYTHON_DIR=%cd%\\win32\\Python) else (set PYTHON_DIR=%cd%\\..\\..\\..\\) \r\n")
			file(APPEND   "${__filename__}" "%PYTHON_DIR%\\python.exe -m pip install --user --upgrade numpy PyQt5\r\n")
			file(APPEND   "${__filename__}" "set PATH=%PYTHON_DIR%;%cd%\\bin;%PATH%\r\n")
			file(APPEND   "${__filename__}" "FOR /F \"tokens=* USEBACKQ\" %%F IN (`%PYTHON_DIR%\\python.exe -c \"import os,PyQt5; print(os.path.dirname(PyQt5.__file__))\"`) DO (SET PyQt5_DIR=%%F)\r\n")
			file(APPEND   "${__filename__}" "set PATH=%PyQt5_DIR%\\Qt\\bin;%PATH%\r\n")
			file(APPEND   "${__filename__}" "set QT_PLUGIN_PATH=%PyQt5_DIR%\\Qt\\plugins\r\n")
			file(APPEND   "${__filename__}" "bin\\\\visusviewer.exe %*\r\n")		
			install(FILES "${__filename__}" DESTINATION .)
					
		elseif (APPLE)
		
			set(__filename__ "${CMAKE_BINARY_DIR}/visusviewer.command")
			file(WRITE    "${__filename__}" "#!/bin/bash\n")
			file(APPEND   "${__filename__}" "this_dir=\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\" )\" && pwd)\n")
			file(APPEND   "${__filename__}" "cd \${this_dir}\n")
			file(APPEND   "${__filename__}" "export PATH=\${this_dir}/bin:\$PATH\n")
			file(APPEND   "${__filename__}" "export QT_PLUGIN_PATH=\${this_dir}/bin/plugins\n")
			file(APPEND   "${__filename__}" "export PYTHONPATH=\${this_dir}:\${this_dir}/bin:\$PYTHONPATH\n")
			file(APPEND   "${__filename__}" "./bin/visusviewer.app/Contents/MacOS/visusviewer\n")
			install(FILES "${__filename__}"  DESTINATION . PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)	
			
			set(__filename__ "${CMAKE_BINARY_DIR}/visusviewer.qt.conf")
			file(WRITE    "${__filename__}" "[Paths]\n")
			file(APPEND   "${__filename__}" "  Plugins=../../../bin/plugins\n")
			install(FILES "${__filename__}" DESTINATION "bin/visusviewer.app/Contents/Resources/qt.conf")
				
		else()
			
			set(__filename__ "${CMAKE_BINARY_DIR}/visusviewer.sh")
			file(WRITE    "${__filename__}" "#!/bin/bash\n")
			file(APPEND   "${__filename__}" "this_dir=\$(cd \"\$(dirname \"\${BASH_SOURCE[0]}\" )\" && pwd)\n")
			file(APPEND   "${__filename__}" "cd \${this_dir}\n")
			file(APPEND   "${__filename__}" "export PATH=\${this_dir}/bin:\$PATH\n")
			file(APPEND   "${__filename__}" "export QT_PLUGIN_PATH=\${this_dir}/bin/plugins\n")
			file(APPEND   "${__filename__}" "export PYTHONPATH=\${this_dir}:\${this_dir}/bin:\$PYTHONPATH\n")
			file(APPEND   "${__filename__}" "./bin/visusviewer\n")
			install(FILES "${__filename__}"  DESTINATION . PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE)			
		
		endif()
		
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
			COMMAND "${PYTHON_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/CMake/deploy.py" --fix-deps --show-deps  
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

               
