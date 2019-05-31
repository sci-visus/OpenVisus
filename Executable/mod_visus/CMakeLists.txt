
if (WIN32)
	AddLibrary(mod_visus SHARED mod_visus.cpp)
	target_link_libraries(mod_visus PUBLIC VisusIdx)
	LinkPythonToExecutable(mod_visus)
	set_target_properties(mod_visus PROPERTIES FOLDER Executable/)

else()
	# this is for conda where cmake 'find_path' seems to have problems
	if (DEFINED APACHE_DIR)
		set(VISUS_MODVISUS      1)
		set(APACHE_INCLUDE_DIRS ${APACHE_DIR}/include)
		set(APACHE_LIBRARIES    ${APACHE_DIR}/lib/libapr-1.so ${APACHE_DIR}/lib/libaprutil-1.so)
		MESSAGE(STATUS "Enabling mod_visus APACHE_DIR=${APACHE_DIR}")
	else()

		set(__search_include_dirs__ 
			/usr/include
			/usr/local/include
			/usr/include/apr-1
			/usr/include/apr-1.0
			/usr/local/include/apr-1
			/usr/local/include/apr-1.0
			/usr/local/apr/include/apr-1
			/usr/include/httpd
			/usr/local/httpd/include
			/usr/local/include/httpd
			/usr/include/apache2
			/usr/local/include/apache2
			/usr/local/apache2/include)

		set(__search_lib_dirs__
			/usr/lib 
			/usr/local/lib 
			/usr/local/apr/lib 
			/usr/local/httpd/lib
			/usr/local/apache2/lib)

		find_path(APR_INCLUDE_DIR     NAMES apr.h      PATHS  ${__search_include_dirs__} )
		find_path(APRUTIL_INCLUDE_DIR NAMES apu.h      PATHS  ${__search_include_dirs__} )
		find_path(HTTPD_INCLUDE_DIR   NAMES httpd.h    PATHS  ${__search_include_dirs__} )
		find_path(APACHE_INCLUDE_DIR  NAMES httpd.h    PATHS  ${__search_include_dirs__} )
		find_library(APR_LIBRARY      NAMES apr-1      PATHS  ${__search_lib_dirs__} )
		find_library(APRUTIL_LIBRARY  NAMES aprutil-1  PATHS  ${__search_lib_dirs__} )

		MARK_AS_ADVANCED(APR_INCLUDE_DIR)
		MARK_AS_ADVANCED(APRUTIL_INCLUDE_DIR)
		MARK_AS_ADVANCED(HTTPD_INCLUDE_DIR)
		MARK_AS_ADVANCED(APACHE_INCLUDE_DIRS)
		MARK_AS_ADVANCED(APR_LIBRARY)	
		MARK_AS_ADVANCED(APRUTIL_LIBRARY)

		MESSAGE(STATUS "APR_INCLUDE_DIR     ${APR_INCLUDE_DIR}")
		MESSAGE(STATUS "APRUTIL_INCLUDE_DIR ${APRUTIL_INCLUDE_DIR}")
		MESSAGE(STATUS "HTTPD_INCLUDE_DIR   ${HTTPD_INCLUDE_DIR}")
		MESSAGE(STATUS "APACHE_INCLUDE_DIR  ${APACHE_INCLUDE_DIR}")
		MESSAGE(STATUS "APR_LIBRARY         ${APR_LIBRARY}")
		MESSAGE(STATUS "APRUTIL_LIBRARY     ${APRUTIL_LIBRARY}")

		if (APR_LIBRARY AND APR_INCLUDE_DIR AND HTTPD_INCLUDE_DIR AND APACHE_INCLUDE_DIR AND APRUTIL_LIBRARY AND APRUTIL_INCLUDE_DIR)
			SET(VISUS_MODVISUS 1)
			set(APACHE_INCLUDE_DIRS ${APR_INCLUDE_DIR} ${APRUTIL_INCLUDE_DIR} ${HTTPD_INCLUDE_DIR} ${APACHE_INCLUDE_DIR})
			set(APACHE_LIBRARIES    ${APR_LIBRARY} ${APRUTIL_LIBRARY})
			message(STATUS "Enabling mod_visus, found apache")
		else()
			SET(VISUS_MODVISUS 0)
			message(STATUS "Disabling mod_visus, cannot find apache")
		endif()

	endif()

	if (VISUS_MODVISUS)
		AddLibrary(mod_visus mod_visus.cpp)
		target_link_libraries(mod_visus PUBLIC VisusIdx)
		LinkPythonToExecutable(mod_visus)
		set_target_properties(mod_visus PROPERTIES FOLDER Executable/)
		target_include_directories(mod_visus PRIVATE ${APACHE_INCLUDE_DIRS})
		target_link_libraries(mod_visus PRIVATE      ${APACHE_LIBRARIES})		
	endif()

endif()
















