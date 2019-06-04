file(GLOB Sources *.h *.cpp)
source_group("" FILES ${Sources})
AddInternalLibrary(tinyxml ${Sources})
set_target_properties(tinyxml PROPERTIES FOLDER "VisusKernelLibs/")