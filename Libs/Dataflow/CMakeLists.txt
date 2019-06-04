file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusDataflow SHARED ${Sources})
target_link_libraries(VisusDataflow PUBLIC VisusKernel)


