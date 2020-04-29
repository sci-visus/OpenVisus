
file(GLOB Sources *.hpp *.c)
source_group("" FILES ${Sources})

add_library(dtl STATIC ${Sources})
set_target_properties(dtl PROPERTIES FOLDER "ExternalLibs/")