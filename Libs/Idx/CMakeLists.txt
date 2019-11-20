
file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusIdx SHARED ${Sources})
target_link_libraries(VisusIdx PUBLIC VisusDb)
target_compile_definitions(VisusIdx  PUBLIC VISUS_IDX=1 PRIVATE VISUS_BUILDING_IDX=1) 
target_include_directories(VisusIdx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
install(DIRECTORY ./include/Visus DESTINATION ${InstallDir}/include/Idx/)


if (VISUS_PYTHON)
	AddSwigLibrary(VisusIdxPy VisusIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusIdxPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()


