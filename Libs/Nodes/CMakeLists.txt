
file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusNodes SHARED ${Sources})
target_link_libraries(VisusNodes PUBLIC VisusIdx VisusDataflow)
target_compile_definitions(VisusNodes  PUBLIC VISUS_NODES=1 PRIVATE VISUS_BUILDING_NODES=1) 
target_include_directories(VisusNodes PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
install(DIRECTORY ./include/Visus DESTINATION ${InstallDir}/include/Nodes/)

if (VISUS_PYTHON)
	AddSwigLibrary(VisusNodesPy VisusNodes ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()


