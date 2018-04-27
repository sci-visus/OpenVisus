file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
add_library(VisusDataflow ${Sources})
target_compile_definitions(VisusDataflow PRIVATE VISUS_BUILDING_VISUSDATAFLOW=1)
target_include_directories(VisusDataflow PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(VisusDataflow PUBLIC VisusKernel)

