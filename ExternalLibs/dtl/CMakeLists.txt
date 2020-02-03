
file(GLOB Sources *.hpp *.c)
source_group("" FILES ${Sources})
AddExternalLibrary(dtl ${Sources})