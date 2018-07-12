DisableAllWarnings()

add_library(lz4 STATIC 
	lz4.c  
	lz4.h  
	lz4frame.c  
	lz4frame.h  
	lz4frame_static.h  
	lz4hc.c  
	lz4hc.h  
	lz4opt.h  
	xxhash.c  
	xxhash.h
)

target_include_directories(lz4 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(lz4 PROPERTIES FOLDER "InternalLibs/")


