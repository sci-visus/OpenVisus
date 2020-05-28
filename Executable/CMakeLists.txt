
add_subdirectory(visus)

if (VISUS_MODVISUS)
	add_subdirectory(mod_visus)
endif()

if (VISUS_GUI)
  add_subdirectory(viewer)
endif()



