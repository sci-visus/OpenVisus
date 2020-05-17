file(GLOB Sources *.h *.cpp *.inl)
source_group("" FILES ${Sources})

add_library(zfp STATIC ${Sources})
set_target_properties(zfp PROPERTIES FOLDER "ExternalLibs/")