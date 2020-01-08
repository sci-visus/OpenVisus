
FILE(GLOB Sources *.h *.cpp)
AddExecutable(visus ${Sources})
target_link_libraries(visus PUBLIC VisusDb)





