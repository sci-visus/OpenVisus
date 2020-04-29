
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp resources/*.glsl resources/*.qrc)

source_group("" FILES ${Sources})

add_library(VisusGuiNodes SHARED ${Sources})
set_target_properties(VisusGuiNodes PROPERTIES FOLDER "")

target_link_libraries(VisusGuiNodes PUBLIC VisusGui VisusDataflow)

option(VISUS_OSPRAY "Enable ospray" OFF)
if (VISUS_OSPRAY)
	find_package(ospray 2.0 REQUIRED)
	find_package(ospcommon  REQUIRED)
	target_compile_definitions(VisusGuiNodes PUBLIC -DVISUS_OSPRAY=1)
	target_link_libraries(VisusGuiNodes PUBLIC ospray::ospray ospcommon::ospcommon)
endif()

target_compile_definitions(VisusGuiNodes  PRIVATE VISUS_BUILDING_VISUSGUINODES=1)
target_include_directories(VisusGuiNodes  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

INSTALL(DIRECTORY ./include/Visus DESTINATION ${OutputDir}/include/GuiNodes/)

AddSwigLibrary(VisusGuiNodesPy VisusGuiNodes ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusGuiNodesPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/include")

