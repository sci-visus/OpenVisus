
file(GLOB Sources include/Visus/*.h src/*.cpp)

source_group("" FILES ${Sources})

add_library(VisusNodes ${Sources})
target_compile_definitions(VisusNodes PRIVATE VISUS_BUILDING_VISUSNODES=1)
target_include_directories(VisusNodes  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(VisusNodes PUBLIC VisusIdx VisusDataflow)


