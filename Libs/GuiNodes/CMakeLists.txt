
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp resources/*.glsl resources/*.qrc)

source_group("" FILES ${Sources})
AddLibrary(VisusGuiNodes SHARED ${Sources})
target_link_libraries(VisusGuiNodes PUBLIC VisusGui VisusDataflow)

if (VISUS_OSPRAY)
	target_compile_definitions(VisusGuiNodes PUBLIC -DVISUS_OSPRAY=1)
	target_link_libraries(VisusGuiNodes PUBLIC ospray::ospray ospcommon::ospcommon)
endif()

AddSwigLibrary(VisusGuiNodesPy VisusGuiNodes ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusGuiNodesPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/include")

