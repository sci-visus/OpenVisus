
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})

add_library(cryptlite STATIC ${Sources})
set_target_properties(cryptlite PROPERTIES FOLDER "ExternalLibs/")
