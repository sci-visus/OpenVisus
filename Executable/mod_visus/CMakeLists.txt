
if (NOT WIN32)

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

	find_library(APR_LIBRARY      NAMES apr-1      PATHS  ${__search_lib_dirs__} )
	find_library(APRUTIL_LIBRARY  NAMES aprutil-1  PATHS  ${__search_lib_dirs__} )

	MARK_AS_ADVANCED(APR_INCLUDE_DIR)
	MARK_AS_ADVANCED(APRUTIL_INCLUDE_DIR)
	MARK_AS_ADVANCED(APACHE_INCLUDE_DIRS)
	MARK_AS_ADVANCED(APR_LIBRARY)	
	MARK_AS_ADVANCED(APRUTIL_LIBRARY)

	MESSAGE(STATUS "APR_INCLUDE_DIR     ${APR_INCLUDE_DIR}")
	MESSAGE(STATUS "APRUTIL_INCLUDE_DIR ${APRUTIL_INCLUDE_DIR}")
	MESSAGE(STATUS "HTTPD_INCLUDE_DIR   ${HTTPD_INCLUDE_DIR}")
	MESSAGE(STATUS "APR_LIBRARY         ${APR_LIBRARY}")
	MESSAGE(STATUS "APRUTIL_LIBRARY     ${APRUTIL_LIBRARY}")

	if (NOT (APR_LIBRARY AND APR_INCLUDE_DIR AND HTTPD_INCLUDE_DIR AND APRUTIL_LIBRARY AND APRUTIL_INCLUDE_DIR))
		message(FATAL_ERROR "cannot find apache")
	endif()
endif()

add_library(mod_visus SHARED mod_visus.cpp)

if (VISUS_PYTHON)
	target_compile_definitions(mod_visus PUBLIC VISUS_PYTHON=1)
	if (WIN32)
		target_include_directories(mod_visus PUBLIC ${Python_INCLUDE_DIRS})
		target_link_libraries(mod_visus      PUBLIC ${Python_LIBRARY_RELEASE})
	else()
		target_link_libraries(mod_visus PUBLIC Python::Python) # Python library for Python embedding
	endif()
endif()

set_target_properties(mod_visus PROPERTIES FOLDER Executable/)
target_link_libraries(mod_visus PUBLIC VisusDb)

if (NOT WIN32)
	target_include_directories(mod_visus PRIVATE ${APR_INCLUDE_DIR} ${APRUTIL_INCLUDE_DIR} ${HTTPD_INCLUDE_DIR} )
	target_link_libraries(mod_visus PRIVATE      ${APR_LIBRARY} ${APRUTIL_LIBRARY})
endif()
















