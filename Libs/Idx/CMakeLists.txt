
file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusIdx ${Sources})
target_link_libraries(VisusIdx PUBLIC VisusDb)

AddSwigLibrary(VisusIdxPy VisusIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusIdxPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include")		