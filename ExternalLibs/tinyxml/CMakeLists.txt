file(GLOB Sources *.h *.cpp)
source_group("" FILES ${Sources})


add_library(tinyxml STATIC ${Sources})
set_target_properties(tinyxml PROPERTIES FOLDER "ExternalLibs/")
