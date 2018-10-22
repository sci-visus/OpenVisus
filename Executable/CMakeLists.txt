
add_subdirectory(visus)

if (WIN32)
  add_subdirectory(mod_visus)
else()
  find_package(APR    QUIET)
  find_package(Apache QUIET)
  if (APR_FOUND AND APACHE_FOUND)
    add_subdirectory(mod_visus)
  else()
    message(STATUS "Disabling mod_visus. APR or Apache missing")
  endif()
endif()

if (VISUS_GUI)
  add_subdirectory(viewer)
ENDIF()



