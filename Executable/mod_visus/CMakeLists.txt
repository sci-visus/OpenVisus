

if (WIN32)

	AddLibrary(mod_visus mod_visus.cpp)
	target_link_libraries(mod_visus PUBLIC VisusIdx)
	LinkPythonToExecutable(mod_visus)
	set_target_properties(mod_visus PROPERTIES FOLDER ${CMAKE_FOLDER_PREFIX}Executable/)

else()

  find_package(APR    QUIET)
  find_package(Apache QUIET)
  
  if (APR_FOUND AND APACHE_FOUND)

		AddLibrary(mod_visus mod_visus.cpp)
		target_link_libraries(mod_visus PUBLIC VisusIdx)
		LinkPythonToExecutable(mod_visus)
		set_target_properties(mod_visus PROPERTIES FOLDER ${CMAKE_FOLDER_PREFIX}Executable/)
		
		target_include_directories(mod_visus PRIVATE  ${APR_INCLUDE_DIR})
		target_include_directories(mod_visus PRIVATE  ${APRUTIL_INCLUDE_DIR})
		target_include_directories(mod_visus PRIVATE  ${HTTPD_INCLUDE_DIR})
		
		target_link_libraries(mod_visus PRIVATE ${APR_LIBRARY})
		target_link_libraries(mod_visus PRIVATE ${APRUTIL_LIBRARY} )		

  else()
  
    message(STATUS "Disabling mod_visus. APR or Apache missing")
  
  endif()
  
endif()
















