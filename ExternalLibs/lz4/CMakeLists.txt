
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})
AddExternalLibrary(lz4 ${Sources})





