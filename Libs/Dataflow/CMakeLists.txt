file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusDataflow ${Sources})
target_link_libraries(VisusDataflow PUBLIC VisusKernel)

