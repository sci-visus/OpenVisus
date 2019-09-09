
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})
AddExternalLibrary(dtl ${Sources})