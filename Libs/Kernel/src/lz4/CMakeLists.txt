
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})
AddInternalLibrary(lz4 ${Sources})
set_target_properties(lz4 PROPERTIES FOLDER "VisusKernelLibs/")





