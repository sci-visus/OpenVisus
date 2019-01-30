DisableAllWarnings()


AddStaticLibrary(zlib  
	adler32.c 
	compress.c 
	crc32.c 
	deflate.c 
	gzclose.c 
	gzlib.c 
	gzread.c  
	gzwrite.c 
	inflate.c 
	infback.c 
	inftrees.c 
	inffast.c 
	trees.c 
	uncompr.c 
	zutil.c
	zconf.h 
	zlib.h 
	crc32.h 
	deflate.h 
	gzguts.h 
	inffast.h 
	inffixed.h 
	inflate.h  
	inftrees.h 
	trees.h 
	zutil.h)

target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

include(CheckTypeSize)
check_type_size(off64_t OFF64_T)
if (HAVE_OFF64_T)
  target_compile_definitions(zlib _LARGEFILE64_SOURCE=1)
endif()

set_target_properties(zlib PROPERTIES FOLDER "InternalLibs/")


