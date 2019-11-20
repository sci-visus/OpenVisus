
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (VISUS_OSPRAY)
	include_directories(${OSPRAY_INCLUDE_DIR})
	target_compile_definitions(VisusKernel PUBLIC -DVISUS_OSPRAY=1)
endif()

file(GLOB Sources include/Visus/*.h src/*.cpp resources/*.glsl resources/*.qrc)
source_group("" FILES ${Sources})
AddLibrary(VisusGuiNodes SHARED ${Sources})
target_link_libraries(VisusGuiNodes PUBLIC VisusGui VisusDataflow)
target_compile_definitions(VisusGuiNodes  PUBLIC VISUS_GUINODES=1 PRIVATE VISUS_BUILDING_GUINODES=1) 
target_include_directories(VisusGuiNodes PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
install(DIRECTORY ./include/Visus DESTINATION ${InstallDir}/include/GuiNodes/)

if (VISUS_OSPRAY)
	target_link_libraries(VisusGuiNodes PUBLIC ${OSPRAY_LIBRARIES})
endif()

if (VISUS_PYTHON)
	AddSwigLibrary(VisusGuiNodesPy VisusGuiNodes ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusGuiNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

