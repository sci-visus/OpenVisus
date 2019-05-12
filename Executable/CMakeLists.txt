
if (VISUS_IDX)
	add_subdirectory(visus)
endif()

if (VISUS_IDX)
	add_subdirectory(mod_visus)
endif()

if (VISUS_APPKIT)
  add_subdirectory(viewer)
endif()



