
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB Sources include/Visus/*.h src/*.cpp resources/*.glsl resources/*.qrc)

source_group("" FILES ${Sources})

add_library(VisusGuiNodes ${Sources})
target_compile_definitions(VisusGuiNodes PRIVATE VISUS_BUILDING_VISUSGUINODES=1)
target_include_directories(VisusGuiNodes  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(VisusGuiNodes PUBLIC VisusGui VisusDataflow)



