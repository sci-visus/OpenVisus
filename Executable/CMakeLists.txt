
add_subdirectory(visus)

if (NOT WIN32 AND NOT APPLE)
	add_subdirectory(mod_visus)
endif()

if (VISUS_GUI)
  add_subdirectory(viewer)
endif()



