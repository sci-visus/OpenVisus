

find_library(APR_LIBRARY      REQUIRED NAMES apr-1)
find_library(APRUTIL_LIBRARY  REQUIRED NAMES aprutil-1)

add_library(mod_visus SHARED mod_visus.cpp)
set_target_properties(mod_visus PROPERTIES FOLDER Executable/)

target_link_libraries(mod_visus PUBLIC VisusDb)

target_include_directories(mod_visus PRIVATE ${APR_INCLUDE_DIR} ${APRUTIL_INCLUDE_DIR} ${HTTPD_INCLUDE_DIR} ${APACHE_INCLUDE_DIR})
target_link_libraries(mod_visus PRIVATE      ${APR_LIBRARY} ${APRUTIL_LIBRARY})		

















