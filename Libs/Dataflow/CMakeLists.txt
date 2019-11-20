file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusDataflow SHARED ${Sources})
target_link_libraries(VisusDataflow PUBLIC VisusKernel)
target_compile_definitions(VisusDataflow  PUBLIC VISUS_DATAFLOW=1 PRIVATE VISUS_BUILDING_DATAFLOW=1) 
target_include_directories(VisusDataflow PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
install(DIRECTORY ./include/Visus DESTINATION ${InstallDir}/include/Dataflow/)

 if (VISUS_PYTHON)
	AddSwigLibrary(VisusDataflowPy VisusDataflow ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusDataflowPy.i
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()


