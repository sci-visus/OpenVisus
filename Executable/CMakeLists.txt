
add_subdirectory(mod_visus)
add_subdirectory(visus)
#add_subdirectory(python) 

if (VISUS_GUI)
  add_subdirectory(viewer)
ENDIF()



