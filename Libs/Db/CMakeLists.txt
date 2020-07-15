
FILE(GLOB RootSources
	./include/Visus/Db.h         ./src/Db.cpp
	./include/Visus/ModVisus.h   ./src/ModVisus.cpp
	include/Visus/VisusConvert.h ./src/VisusConvert.cpp
	src/SelfTest*.cpp
	src/tutorial*.cpp)
source_group("" FILES ${RootSources})

SET(QuerySources
	./include/Visus/LogicSamples.h ./src/LogicSamples.cpp
	./include/Visus/Query.h        ./src/Query.cpp
	./include/Visus/BlockQuery.h   ./src/BlockQuery.cpp
	./include/Visus/BoxQuery.h     ./src/BoxQuery.cpp
	./include/Visus/PointQuery.h   ./src/PointQuery.cpp)
source_group("Query" FILES ${QuerySources})

SET(DatasetSources
	./include/Visus/Dataset.h             ./src/Dataset.cpp
	./include/Visus/DatasetArrayPlugin.h  ./src/DatasetArrayPlugin.cpp
	./include/Visus/DatasetBitmask.h      ./src/DatasetBitmask.cpp
	./include/Visus/DatasetTimesteps.h    ./src/DatasetTimesteps.cpp
	./include/Visus/GoogleMapsDataset.h   ./src/GoogleMapsDataset.cpp)
source_group("Dataset" FILES ${DatasetSources})

SET(AccessSources
	./include/Visus/Access.h             ./src/Access.cpp
	./include/Visus/CloudStorageAccess.h ./src/CloudStorageAccess.cpp
	./include/Visus/DiskAccess.h         ./src/DiskAccess.cpp
	./include/Visus/FilterAccess.h       ./src/FilterAccess.cpp
	./include/Visus/ModVisusAccess.h     ./src/ModVisusAccess.cpp
	./include/Visus/MultiplexAccess.h    ./src/MultiplexAccess.cpp
	./include/Visus/RamAccess.h          ./src/RamAccess.cpp
	./include/Visus/OnDemandAccess.h     ./src/OnDemandAccess.cpp
	./include/Visus/GoogleMapsAccess.h   ./src/GoogleMapsAccess.cpp
	)
source_group("Access" FILES ${AccessSources})

FILE(GLOB IdxSources
	include/Visus/Idx*.h src/Idx*.cpp )
source_group("Idx" FILES ${IdxSources})

file(GLOB Sources include/Visus/*.h src/*.cpp)
add_library(VisusDb SHARED ${Sources})
set_target_properties(VisusDb PROPERTIES FOLDER "")
target_link_libraries(VisusDb PUBLIC VisusKernel)
target_compile_definitions(VisusDb  PRIVATE VISUS_BUILDING_VISUSDB=1)
target_include_directories(VisusDb  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

