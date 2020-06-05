
FILE(GLOB RootSources
	./include/Visus/Db.h
	./src/Db.cpp
	./include/Visus/ModVisus.h
	./src/ModVisus.cpp
	src/tutorial*.cpp
	src/SelfTest*.cpp
	include/Visus/VisusConvert.h
	src/VisusConvert.cpp)

SET(QuerySources
	./include/Visus/LogicSamples.h
	./src/LogicSamples.cpp
	./include/Visus/Query.h
	./src/Query.cpp
	./include/Visus/BlockQuery.h
	./src/BlockQuery.cpp
	./include/Visus/BoxQuery.h
	./src/BoxQuery.cpp
	./include/Visus/PointQuery.h
	./src/PointQuery.cpp)

SET(DatasetSources
	./include/Visus/Dataset.h
	./include/Visus/DatasetArrayPlugin.h  
	./include/Visus/DatasetBitmask.h
	./include/Visus/DatasetFilter.h
	./include/Visus/DatasetTimesteps.h
	./include/Visus/GoogleMapsDataset.h
	./src/Dataset.cpp
	./src/DatasetArrayPlugin.cpp
	./src/DatasetBitmask.cpp
	./src/DatasetFilter.cpp
	./src/DatasetTimesteps.cpp
	./src/GoogleMapsDataset.cpp)

SET(AccessSources
	./include/Visus/Access.h
	./include/Visus/CloudStorageAccess.h
	./include/Visus/DiskAccess.h
	./include/Visus/FilterAccess.h
	./include/Visus/ModVisusAccess.h
	./include/Visus/MultiplexAccess.h
	./include/Visus/RamAccess.h
	./src/Access.cpp
	./src/CloudStorageAccess.cpp
	./src/DiskAccess.cpp
	./src/ModVisusAccess.cpp
	./src/FilterAccess.cpp
	./src/MultiplexAccess.cpp
	./src/RamAccess.cpp
	./include/Visus/OnDemandAccess.h
	./src/OnDemandAccess.cpp)

FILE(GLOB IdxSources
	include/Visus/Idx*.h          
	src/Idx*.cpp   
	src/LogicSamples.cpp )

source_group(""         FILES ${RootSources})
source_group("Access"   FILES ${AccessSources})
source_group("Dataset"  FILES ${DatasetSources})
source_group("Query"    FILES ${QuerySources})
source_group("Idx"      FILES ${IdxSources})

file(GLOB Sources include/Visus/*.h src/*.cpp)
add_library(VisusDb SHARED ${Sources})
set_target_properties(VisusDb PROPERTIES FOLDER "")
target_link_libraries(VisusDb PUBLIC VisusKernel)
target_compile_definitions(VisusDb  PRIVATE VISUS_BUILDING_VISUSDB=1)
target_include_directories(VisusDb  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

