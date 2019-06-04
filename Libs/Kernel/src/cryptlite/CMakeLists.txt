
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})
AddInternalLibrary(cryptlite ${Sources})
set_target_properties(cryptlite PROPERTIES FOLDER "VisusKernelLibs/")