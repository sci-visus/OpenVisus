file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusDataflow SHARED ${Sources})
target_link_libraries(VisusDataflow PUBLIC VisusKernel)

AddSwigLibrary(VisusDataflowPy VisusDataflow ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusDataflowPy.i
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/include")


