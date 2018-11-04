
add_library(mod_visus mod_visus.cpp)
target_link_libraries(mod_visus PUBLIC VisusIdx)
set_target_properties(mod_visus PROPERTIES FOLDER ${CMAKE_FOLDER_PREFIX}Executable/)

if (NOT WIN32)
  target_link_libraries(mod_visus PUBLIC ${PYTHON_LIBRARY})
endif()

if (NOT WIN32)

  string(REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+" "\\1" APACHE_MAJOR_VERSION ${APACHE_VERSION} )
  string(REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+" "\\1" APACHE_MINOR_VERSION ${APACHE_VERSION} )
  string(REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+)" "\\1" APACHE_PATCH_VERSION ${APACHE_VERSION} )
  
  add_definitions(-DAPACHE_MAJOR_VERSION=${APACHE_MAJOR_VERSION})
  add_definitions(-DAPACHE_MINOR_VERSION=${APACHE_MINOR_VERSION})
  add_definitions(-DAPACHE_PATCH_VERSION=${APACHE_PATCH_VERSION})

  target_include_directories(mod_visus  PRIVATE  ${APR_INCLUDE_DIR}  ${HTTPD_INCLUDE_DIR}  ${APRUTIL_INCLUDE_DIR})
   
  target_link_libraries(mod_visus PRIVATE ${APR_LIBRARY} ${APRUTIL_LIBRARY} )
  
  if (APPLE)
		set_target_properties(mod_visus PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
  else()
		target_include_directories(mod_visus PRIVATE  /usr/include/openssl  /usr/include/xmltok)   
  endif()
    
endif()

InstallLibrary(mod_visus)










