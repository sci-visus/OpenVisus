
file(GLOB Sources  include/Visus/*.h  src/*.cpp)
	
source_group("" FILES ${Sources})
AddLibrary(VisusXIdx SHARED ${Sources})
target_link_libraries(VisusXIdx PUBLIC VisusKernel)
target_compile_definitions(VisusXIdx  PUBLIC VISUS_XIDX=1 PRIVATE VISUS_BUILDING_XIDX=1) 
target_include_directories(VisusXIdx PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
install(DIRECTORY ./include/Visus DESTINATION ${InstallDir}/include/XIdx/)

if (VISUS_PYTHON)
	AddSwigLibrary(VisusXIdxPy VisusXIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusXIdx.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()



