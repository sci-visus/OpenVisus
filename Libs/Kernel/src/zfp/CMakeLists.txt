
file(GLOB Sources *.h *.cpp *.inl)
source_group("" FILES ${Sources})
AddInternalLibrary(zfp ${Sources})
set_target_properties(zfp PROPERTIES FOLDER "VisusKernelLibs/")





