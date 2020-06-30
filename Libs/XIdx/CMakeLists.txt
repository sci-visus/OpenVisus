
file(GLOB Sources include/Visus/*.h  src/*.cpp)
source_group("" FILES ${Sources})

add_library(VisusXIdx SHARED ${Sources})

set_target_properties(VisusXIdx PROPERTIES FOLDER "")
target_link_libraries(VisusXIdx PUBLIC VisusKernel)
target_compile_definitions(VisusXIdx  PRIVATE VISUS_BUILDING_VISUSXIDX=1)
target_include_directories(VisusXIdx  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

INSTALL(DIRECTORY include/Visus     DESTINATION \${CMAKE_INSTALL_CONFIG_NAME}/OpenVisus/include/XIdx/)