
file(GLOB Sources include/Visus/*.h src/*.cpp)

source_group("" FILES ${Sources})

AddLibrary(VisusIdx ${Sources})
target_link_libraries(VisusIdx PUBLIC VisusDb)


