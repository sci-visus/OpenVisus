
file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp)
source_group("" FILES ${Sources})

add_library(VisusNodes SHARED ${Sources})
set_target_properties(VisusNodes PROPERTIES FOLDER "")

target_link_libraries(VisusNodes PUBLIC VisusDb VisusDataflow)

target_compile_definitions(VisusNodes  PRIVATE VISUS_BUILDING_VISUSNODES=1)
target_include_directories(VisusNodes  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

InstallDirectory(./include/Visus ./include/Nodes/)

if (VISUS_PYTHON)
	AddSwigLibrary(VisusNodesPy VisusNodes ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusNodesPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()


