
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})
AddInternalLibrary(dtl ${Sources})
set_target_properties(dtl PROPERTIES FOLDER "VisusKernelLibs/")