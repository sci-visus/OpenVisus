
file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusNodes ${Sources})
target_link_libraries(VisusNodes PUBLIC VisusIdx VisusDataflow)

AddSwigLibrary(VisusNodesPy 
	VisusNodes 
	${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusNodesPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include")