
file(GLOB Sources *.h *.c)
source_group("" FILES ${Sources})
AddExternalLibrary(cryptlite ${Sources})