
file(GLOB Sources  include/Visus/*.h  src/*.cpp)
	
source_group("" FILES ${Sources})
AddLibrary(VisusXIdx SHARED ${Sources})
target_link_libraries(VisusXIdx PUBLIC VisusKernel)



