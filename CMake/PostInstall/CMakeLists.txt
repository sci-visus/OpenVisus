

if (NOT PYTHON_TAG)
  set(PYTHON_TAG cp${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR})
endif()

if (NOT PLAT_NAME)
 	if (WIN32)
 		set(PLAT_NAME win_amd64)
 	elseif (APPLE)
 		string(SUBSTRING ${APPLE_OSX_VERSION} 0 2 __major__)
 		string(SUBSTRING ${APPLE_OSX_VERSION} 3 2 __minor__)
 		set(PLAT_NAME macosx_${__major__}_${__minor__}_x86_64)
 	else()
 		execute_process(COMMAND lsb_release -is OUTPUT_VARIABLE __id__   OUTPUT_STRIP_TRAILING_WHITESPACE)
 		execute_process(COMMAND lsb_release -cs OUTPUT_VARIABLE __name__ OUTPUT_STRIP_TRAILING_WHITESPACE)
 		set(PLAT_NAME "${__id__}.${__name__}")
 	endif()
endif()

install(CODE "
	execute_process( 
		 COMMAND \"${PYTHON_EXECUTABLE}\" \"${CMAKE_CURRENT_SOURCE_DIR}/../BundleUtils.py\" --Qt5_DIR \"${Qt5_DIR}\" --OPENSSL_ROOT_DIR \"${OPENSSL_ROOT_DIR}\" --python-tag ${PYTHON_TAG} --plat-name ${PLAT_NAME} --cmake-post-install
		 WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}\")
")

