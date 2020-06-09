

add_subdirectory(visus)

if (VISUS_GUI)
  add_subdirectory(viewer)
endif()

if (VISUS_MODVISUS)
	add_subdirectory(mod_visus)
endif()





