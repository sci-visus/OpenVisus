
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})

add_library(lz4 STATIC ${Sources})
set_target_properties(lz4 PROPERTIES FOLDER "ExternalLibs/")







