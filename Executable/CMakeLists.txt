
if (VISUS_DB)
	add_subdirectory(visus)
endif()

if (NOT WIN32 AND NOT APPLE)
	add_subdirectory(mod_visus)
endif()

if (VISUS_APPKIT)
  add_subdirectory(viewer)
endif()



