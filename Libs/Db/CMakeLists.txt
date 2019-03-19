
SET(RootSources
	./include/Visus/Db.h
	./include/Visus/ModVisus.h
	./src/Db.cpp
	./src/ModVisus.cpp
)

SET(QuerySources
	./include/Visus/LogicBox.h
	./include/Visus/BlockQuery.h
	./include/Visus/Query.h
	./src/LogicBox.cpp
	./src/BlockQuery.cpp
	./src/Query.cpp
)

SET(DatasetSources
	./include/Visus/Dataset.h
	./include/Visus/DatasetArrayPlugin.h  
	./include/Visus/DatasetBitmask.h
	./include/Visus/DatasetFilter.h
	./include/Visus/DatasetTimeSteps.h
	./include/Visus/GoogleMapsDataset.h
	./src/Dataset.cpp
	./src/DatasetArrayPlugin.cpp
	./src/DatasetBitmask.cpp
	./src/DatasetFilter.cpp
	./src/DatasetTimeSteps.cpp
	./src/GoogleMapsDataset.cpp
)

SET(SceneSources
	./include/Visus/Scene.h
	./src/Scene.cpp
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
source_group("Scene"    FILES ${SceneSources})
source_group("Query"    FILES ${QuerySources})

file(GLOB Sources include/Visus/*.h include/Visus/*.i src/*.cpp)

AddLibrary(VisusDb ${Sources})
target_link_libraries(VisusDb PUBLIC VisusKernel)

AddSwigLibrary(VisusDbPy VisusDb ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusDbPy.i 
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include"
	"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include")