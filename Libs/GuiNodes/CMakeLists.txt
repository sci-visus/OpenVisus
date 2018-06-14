
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB Sources include/Visus/*.h src/*.cpp resources/*.glsl resources/*.qrc)

source_group("" FILES ${Sources})

AddVisusLibrary(VisusGuiNodes ${Sources})
target_link_libraries(VisusGuiNodes PUBLIC VisusGui VisusDataflow)



