
add_subdirectory(Kernel)
add_subdirectory(Db)

if (VISUS_DATAFLOW)

	add_subdirectory(Dataflow)
	add_subdirectory(Nodes)

	if (VISUS_GUI)
		add_subdirectory(Gui)
	endif()
	
endif()

if (VISUS_PYTHON)
	add_subdirectory(swig)
endif()


if(VISUS_WEAVING)
	add_subdirectory(VisusWeaving)
endif()	

# NOTE: I am not using the IDX2 original CMakeList.txt so I have more "freedom" to change stuff
#       but if anything is going to change in IDX, you MUST keep this up-to-date
if (VISUS_IDX2)
	FILE(GLOB Idx2Sources         ${CMAKE_CURRENT_SOURCE_DIR}/IDX2/Source/Core/*.h ${CMAKE_CURRENT_SOURCE_DIR}/IDX2/Source/Core/*.cpp
		                          ${CMAKE_CURRENT_SOURCE_DIR}/IDX2/Source/idx2.h ${CMAKE_CURRENT_SOURCE_DIR}/IDX2/Source/idx2.cpp)
	list(REMOVE_ITEM Idx2Sources  ${CMAKE_CURRENT_SOURCE_DIR}/IDX2/Source/Core/Core.cpp)
	add_library(idx2 STATIC ${Idx2Sources})
	target_compile_features(idx2    PUBLIC cxx_std_17)
	target_compile_definitions(idx2 PUBLIC -DVISUS_IDX2=1)
	target_compile_definitions(idx2 PUBLIC -Didx2_Avx2)
	target_compile_definitions(idx2 PUBLIC "$<$<CONFIG:DEBUG>:idx2_Slow>")
	target_include_directories(idx2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/IDX2)
	if (MSVC)
		target_compile_definitions(idx2 PUBLIC -D_CRT_SECURE_NO_WARNINGS)
		target_compile_options(idx2 PUBLIC /arch:AVX2 /Zc:preprocessor /Zc:__cplusplus /wd5105)
		target_link_options(idx2 PUBLIC dbghelp.lib)
	elseif (UNIX)
		target_compile_options(idx2 PUBLIC -Wno-format-zero-length -mavx2)
	endif()
	set_target_properties(idx2 PROPERTIES FOLDER "ExternalLibs/")
endif()