
FILE(GLOB RootSources
	./include/Visus/Db.h           ./src/Db.cpp
	./include/Visus/ModVisus.h     ./src/ModVisus.cpp
	./include/Visus/VisusConvert.h ./src/VisusConvert.cpp
	./include/Visus/Minimal.h      ./src/Minimal.cpp)
source_group("" FILES ${RootSources})

FILE(GLOB QuerySources ./include/Visus/*Query.h ./src/*Query.cpp ./include/Visus/LogicSamples.h ./src/LogicSamples.cpp)
source_group("Query" FILES ${QuerySources})

FILE(GLOB DatasetSources  ./include/Visus/Dataset*.h  ./src/Dataset*.cpp ./include/Visus/GoogleMapsDataset.h   ./src/GoogleMapsDataset.cpp)
source_group("Dataset" FILES ${DatasetSources})

FILE(GLOB AccessSources ./include/Visus/*Access.h ./src/*Access.cpp)
source_group("Access" FILES ${AccessSources})

FILE(GLOB IdxSources include/Visus/Idx*.h src/Idx*.cpp )
source_group("Idx" FILES ${IdxSources})

FILE(GLOB TutorialSources ${CMAKE_SOURCE_DIR}/Samples/cpp/*.cpp src/SelfTest*.cpp)
source_group("Samples\\cpp" FILES ${TutorialSources})

file(GLOB Sources include/Visus/*.h src/*.cpp ${TutorialSources})
add_library(VisusDb ${Sources})

set_target_properties(VisusDb PROPERTIES FOLDER "")
target_link_libraries(VisusDb PUBLIC VisusKernel)
target_compile_definitions(VisusDb  PRIVATE VISUS_BUILDING_VISUSDB=1)

if (VISUS_IDX2)
	target_compile_definitions(VisusDb PUBLIC VISUS_IDX2=1)
	target_link_libraries(VisusDb PUBLIC idx2_lib)
endif()

target_include_directories(VisusDb  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

