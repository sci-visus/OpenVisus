file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})

add_library(zlib STATIC ${Sources})
set_target_properties(zlib PROPERTIES FOLDER "ExternalLibs/")

include(CheckTypeSize)
check_type_size(off64_t OFF64_T)
if (HAVE_OFF64_T)
  target_compile_definitions(zlib _LARGEFILE64_SOURCE=1)
endif()
