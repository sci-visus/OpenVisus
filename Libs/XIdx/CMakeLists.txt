
file(GLOB Sources 
	include/Visus/*.h 
	include/Visus/*.i 
	src/*.cpp
	)
	
source_group("" FILES ${Sources})
AddLibrary(VisusXIdx ${Sources})
target_link_libraries(VisusXIdx PUBLIC VisusKernel)

AddSwigLibrary(VisusXIdxPy 
	VisusXIdx 
	${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusXIdx.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/include")

#add_subdirectory(examples/cpp)
