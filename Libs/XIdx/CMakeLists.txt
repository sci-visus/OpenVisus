
file(GLOB Sources  include/Visus/*.h  src/*.cpp)
	
source_group("" FILES ${Sources})
AddLibrary(VisusXIdx SHARED ${Sources})
target_link_libraries(VisusXIdx PUBLIC VisusKernel)

if (VISUS_PYTHON)
	AddSwigLibrary(VisusXIdxPy VisusXIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusXIdx.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()



