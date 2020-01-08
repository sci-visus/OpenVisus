
if (VISUS_DB)
	add_subdirectory(visus)
endif()

if (VISUS_DB)
	add_subdirectory(mod_visus)
endif()

if (VISUS_APPKIT)
  add_subdirectory(viewer)
endif()



