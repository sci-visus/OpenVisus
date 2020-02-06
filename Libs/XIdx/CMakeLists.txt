
file(GLOB Sources  include/Visus/*.h  src/*.cpp include/Visus/*.i)
	
source_group("" FILES ${Sources})
AddLibrary(VisusXIdx SHARED ${Sources})
target_link_libraries(VisusXIdx PUBLIC VisusKernel)

AddSwigLibrary(VisusXIdxPy VisusXIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusXIdx.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/include")



