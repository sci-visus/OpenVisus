
add_subdirectory(visus)

if (VISUS_GUI)
  add_subdirectory(viewer)
endif()

if (VISUS_MODVISUS)
	add_subdirectory(mod_visus)
endif()

if (VISUS_HDF5)
	add_subdirectory(hdf5_plugin)
endif()

if (VISUS_IDX2)
	add_subdirectory(idx2)
endif()	
	



