
SET(RootSources
	./include/Visus/Db.h
	./include/Visus/ModVisus.h
	./src/Db.cpp
	./src/ModVisus.cpp
)

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
	./src/PointQuery.cpp
)

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
	./src/GoogleMapsDataset.cpp
)

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
	./src/OnDemandAccess.cpp
)

source_group(""         FILES ${RootSources})
source_group("Access"   FILES ${AccessSources})
source_group("Dataset"  FILES ${DatasetSources})
source_group("Query"    FILES ${QuerySources})

file(GLOB Sources include/Visus/*.h src/*.cpp)

AddLibrary(VisusDb SHARED ${Sources})
target_link_libraries(VisusDb PUBLIC VisusKernel)
target_compile_definitions(VisusDb  PUBLIC VISUS_DB=1 PRIVATE VISUS_BUILDING_DB=1) 
target_include_directories(VisusDb PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
install(DIRECTORY ./include/Visus DESTINATION ${InstallDir}/include/Db/)


if (VISUS_PYTHON)
	AddSwigLibrary(VisusDbPy VisusDb ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusDbPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()

