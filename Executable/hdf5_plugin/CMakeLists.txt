source_group("" FILES openvisus_vol.cpp openvisus_vol.h)
add_library(openvisus_vol SHARED openvisus_vol.cpp openvisus_vol.h)

target_include_directories(openvisus_vol PUBLIC ${HDF5_INCLUDE_DIRS})
target_link_libraries(openvisus_vol      PUBLIC ${HDF5_LIBRARIES} VisusDb)

if (VISUS_PYTHON)
	target_compile_definitions(openvisus_vol PUBLIC VISUS_PYTHON=1)
	if (WIN32)
		target_include_directories(openvisus_vol PUBLIC ${Python_INCLUDE_DIRS})
		target_link_libraries(openvisus_vol      PUBLIC ${Python_LIBRARY_RELEASE})
	else()
		target_link_libraries(openvisus_vol PUBLIC Python::Python) # Python library for Python embedding
	endif()
endif()

source_group("" FILES main.cpp)
add_executable(openvisus_vol_cli main.cpp README.md)
target_include_directories(openvisus_vol_cli PUBLIC ${HDF5_INCLUDE_DIRS})
target_link_libraries(openvisus_vol_cli      PUBLIC ${HDF5_LIBRARIES})
add_dependencies(openvisus_vol_cli openvisus_vol)

set_target_properties(openvisus_vol PROPERTIES FOLDER "HDF5")
set_target_properties(openvisus_vol_cli     PROPERTIES FOLDER "HDF5")




