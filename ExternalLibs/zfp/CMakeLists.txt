file(GLOB Sources *.h *.cpp *.inl)
source_group("" FILES ${Sources})
AddExternalLibrary(zfp ${Sources})