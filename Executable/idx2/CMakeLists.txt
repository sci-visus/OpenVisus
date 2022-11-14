FILE(GLOB Sources ReadMe.md *.h *.cpp)
add_executable(visus_idx2 ${Sources})
target_link_libraries(visus_idx2 PUBLIC VisusDb)
set_target_properties(visus_idx2 PROPERTIES FOLDER "Executable/")






