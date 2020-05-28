
file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp)
source_group("" FILES ${Sources})

add_library(VisusNodes SHARED ${Sources})
set_target_properties(VisusNodes PROPERTIES FOLDER "")

target_link_libraries(VisusNodes PUBLIC VisusDb VisusDataflow)

target_compile_definitions(VisusNodes  PRIVATE VISUS_BUILDING_VISUSNODES=1)
target_include_directories(VisusNodes  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

CopyToBinaryDirectory(TARGET VisusNodes DIRECTORY ./include/Visus DESTINATION ./include/Nodes/Visus)

set(SwigFile ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusNodesPy.i)
set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")
set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include")
set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include")
set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Idx/include")
set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/include")
AddSwigLibrary(VisusNodesPy VisusNodes ${SwigFile})



