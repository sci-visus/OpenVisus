
file(GLOB Sources  include/Visus/*.h  include/Visus/*.i src/*.cpp)
	
source_group("" FILES ${Sources})

add_library(VisusXIdx SHARED ${Sources})
set_target_properties(VisusXIdx PROPERTIES FOLDER "")

target_link_libraries(VisusXIdx PUBLIC VisusKernel)

target_compile_definitions(VisusXIdx  PRIVATE VISUS_BUILDING_VISUSXIDX=1)
target_include_directories(VisusXIdx  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

INSTALL(DIRECTORY ./include/Visus DESTINATION ${ConfigName}/${PROJECT_NAME}/include/XIdx/)

if (VISUS_PYTHON)
    AddSwigLibrary(VisusXIdxPy VisusXIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusXIdx.i 
	    "-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	    "-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()



