
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (VISUS_OSPRAY)
	include_directories(${OSPRAY_INCLUDE_DIR})
	target_compile_definitions(VisusKernel PUBLIC -DVISUS_OSPRAY=1)
endif()

file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp resources/*.glsl resources/*.qrc)
source_group("" FILES ${Sources})
AddLibrary(VisusGuiNodes ${Sources})
target_link_libraries(VisusGuiNodes PUBLIC VisusGui VisusDataflow)

if (VISUS_OSPRAY)
	target_link_libraries(VisusGuiNodes PUBLIC ${OSPRAY_LIBRARIES})
endif()

AddSwigLibrary(VisusGuiNodesPy 
	VisusGuiNodes 
	OpenVisus
	${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusGuiNodesPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../GuiNodes/include")
