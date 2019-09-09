
file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusNodes SHARED ${Sources})
target_link_libraries(VisusNodes PUBLIC VisusIdx VisusDataflow)

if (VISUS_PYTHON)
	AddSwigLibrary(VisusNodes ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")

endif()


