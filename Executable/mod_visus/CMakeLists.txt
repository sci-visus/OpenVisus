
AddLibrary(mod_visus mod_visus.cpp)
target_link_libraries(mod_visus PUBLIC VisusIdx)
LinkPythonToExecutable(mod_visus)

set_target_properties(mod_visus PROPERTIES FOLDER ${CMAKE_FOLDER_PREFIX}Executable/)

if (NOT WIN32)

  string(REGEX REPLACE "([0-9]+).[0-9]+.[0-9]+" "\\1" APACHE_MAJOR_VERSION ${APACHE_VERSION} )
  string(REGEX REPLACE "[0-9]+.([0-9]+).[0-9]+" "\\1" APACHE_MINOR_VERSION ${APACHE_VERSION} )
  string(REGEX REPLACE "[0-9]+.[0-9]+.([0-9]+)" "\\1" APACHE_PATCH_VERSION ${APACHE_VERSION} )
  
  target_compile_definitions(mod_visus PRIVATE -DAPACHE_MAJOR_VERSION=${APACHE_MAJOR_VERSION})
  target_compile_definitions(mod_visus PRIVATE -DAPACHE_MINOR_VERSION=${APACHE_MINOR_VERSION})
  target_compile_definitions(mod_visus PRIVATE -DAPACHE_PATCH_VERSION=${APACHE_PATCH_VERSION})
  
  target_include_directories(mod_visus PRIVATE  ${APR_INCLUDE_DIR})
  target_include_directories(mod_visus PRIVATE  ${HTTPD_INCLUDE_DIR})
  target_include_directories(mod_visus PRIVATE  ${APRUTIL_INCLUDE_DIR})
   
  target_link_libraries(mod_visus PRIVATE ${APR_LIBRARY})
  target_link_libraries(mod_visus PRIVATE ${APRUTIL_LIBRARY} )
  
endif()









