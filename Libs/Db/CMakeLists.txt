
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

# NOTE: I am not using the IDX2 original CMakeList.txt so I have more "freedom" to change stuff
#       but if anything is going to change in IDX, you MUST keep this up-to-date
if (VISUS_IDX2)
	FILE(GLOB Idx2Sources  
		${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/Core/*.h ${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/Core/*.cpp
		${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/idx2.h   
		${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/idx2.cpp
		${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/Applications/idx2App.cpp # IDX2 does not support dynamic libraries, so I am compiling here
	)
	list(REMOVE_ITEM Idx2Sources  ${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/Core/Core.cpp)
	source_group("Idx2" FILES ${Idx2Sources})
endif()

file(GLOB Sources include/Visus/*.h src/*.cpp ${TutorialSources} ${Idx2Sources})

add_library(VisusDb ${Sources})

set_target_properties(VisusDb PROPERTIES FOLDER "")
target_link_libraries(VisusDb PUBLIC VisusKernel)
target_compile_definitions(VisusDb  PRIVATE VISUS_BUILDING_VISUSDB=1)

target_include_directories(VisusDb PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# NOTE: I am not using the IDX2 original CMakeList.txt so I have more "freedom" to change stuff
#       but if anything is going to change in IDX, you MUST keep this up-to-date
if (VISUS_IDX2)

	# some code needs C++17
	target_compile_features(VisusDb PRIVATE cxx_std_17)

	# enable idx2
	target_compile_definitions(VisusDb PUBLIC -DVISUS_IDX2=1)

	# less precise but more portable clock functions
	target_compile_definitions(VisusDb PRIVATE -Didx2_CTimer)
	
	# enable assert in debug mode
	target_compile_definitions(VisusDb PRIVATE "$<$<CONFIG:DEBUG>:idx2_Slow>") 
	
	# includes
	target_include_directories(VisusDb PRIVATE 
		${CMAKE_SOURCE_DIR}/Libs/IDX2/Source 
		${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/Core/stla)

	if (MSVC)
		target_compile_definitions(VisusDb PRIVATE -D_CRT_SECURE_NO_WARNINGS  -DNOMINMAX)
		target_compile_options(VisusDb PRIVATE /Zc:preprocessor /Zc:__cplusplus /wd5105)
		target_link_options(VisusDb PRIVATE dbghelp.lib)
	else()
		target_compile_options(VisusDb PRIVATE -Wno-format-zero-length)
	endif()

	# AVX2 platform is disabled by default
	if (VISUS_IDX2_AVX2)
		target_compile_definitions(VisusDb PRIVATE -Didx2_Avx2)
		if (MSVC)
			target_compile_options(VisusDb PRIVATE /arch:AVX2)
		elseif (CLANG)
			target_compile_options(VisusDb PRIVATE -mavx2)
		else()
			target_compile_options(VisusDb PRIVATE -march=core-avx2)
		endif()

	endif()
endif()

