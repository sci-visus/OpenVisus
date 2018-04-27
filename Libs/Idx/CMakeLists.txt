
file(GLOB Sources include/Visus/*.h src/*.cpp)

source_group("" FILES ${Sources})

add_library(VisusIdx ${Sources})
target_compile_definitions(VisusIdx PRIVATE VISUS_BUILDING_VISUSIDX=1)
target_include_directories(VisusIdx  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(VisusIdx PUBLIC VisusDb)


