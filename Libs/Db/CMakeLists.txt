
SET(RootSources
	./include/Visus/Db.h
	./include/Visus/ModVisus.h
	./src/Db.cpp
	./src/ModVisus.cpp
)

SET(QuerySources
	./include/Visus/BaseQuery.h
	./include/Visus/BlockQuery.h
	./include/Visus/Query.h
	./src/BaseQuery.cpp
	./src/BlockQuery.cpp
	./src/Query.cpp
)

SET(DatasetSources
	./include/Visus/Dataset.h
	./include/Visus/DatasetArrayPlugin.h  
	./include/Visus/DatasetBitmask.h
	./include/Visus/DatasetFilter.h
	./include/Visus/DatasetTimeSteps.h
	./include/Visus/LegacyDataset.h
	./src/Dataset.cpp
	./src/DatasetArrayPlugin.cpp
	./src/DatasetBitmask.cpp
	./src/DatasetFilter.cpp
	./src/DatasetTimeSteps.cpp
	./src/LegacyDataset.cpp
)

SET(SceneSources
	./include/Visus/Scene.h
	./src/Scene.cpp
)

SET(AccessSources
	./include/Visus/Access.h
	./include/Visus/NetworkAccess.h
	./include/Visus/CloudStorageAccess.h
	./include/Visus/DiskAccess.h
	./include/Visus/FilterAccess.h
	./include/Visus/ModVisusAccess.h
	./include/Visus/MultiplexAccess.h
	./include/Visus/RamAccess.h
	./src/Access.cpp
	./src/NetworkAccess.cpp
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

file(GLOB Sources include/Visus/*.h src/*.cpp)

AddVisusLibrary(VisusDb ${Sources})
target_link_libraries(VisusDb PUBLIC VisusKernel)
