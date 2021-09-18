source_group("" FILES plugin.cpp plugin.h)
add_library(openvisus_vol plugin.cpp plugin.h)

target_compile_definitions(openvisus_vol PRIVATE -DENABLE_EXT_PASSTHRU_LOGGING=1)
target_include_directories(openvisus_vol PUBLIC ${HDF5_INCLUDE_DIRS})
target_link_libraries(openvisus_vol      PUBLIC ${HDF5_LIBRARIES})

source_group("" FILES main.cpp)
add_executable(hdf5-test main.cpp README.md)
target_include_directories(hdf5-test PUBLIC ${HDF5_INCLUDE_DIRS})
target_link_libraries(hdf5-test      PUBLIC VisusDb ${HDF5_LIBRARIES})

add_dependencies(hdf5-test openvisus_vol)


set_target_properties(openvisus_vol PROPERTIES FOLDER "HDF5")
set_target_properties(hdf5-test     PROPERTIES FOLDER "HDF5")




