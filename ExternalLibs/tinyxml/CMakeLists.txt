file(GLOB Sources *.h *.cpp)
source_group("" FILES ${Sources})
AddExternalLibrary(tinyxml ${Sources})