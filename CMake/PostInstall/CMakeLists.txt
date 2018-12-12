
install(CODE "
	execute_process( 
		 COMMAND \"${PYTHON_EXECUTABLE}\" \"${CMAKE_CURRENT_SOURCE_DIR}/../BundleUtils.py\" --Qt5_DIR \"${Qt5_DIR}\" --OPENSSL_ROOT_DIR \"${OPENSSL_ROOT_DIR}\" --cmake-post-install
		 WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}\")
")

