
file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusNodes SHARED ${Sources})
target_link_libraries(VisusNodes PUBLIC VisusIdx VisusDataflow)
