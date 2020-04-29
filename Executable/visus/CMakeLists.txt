
FILE(GLOB Sources *.h *.cpp)
add_executable(visus ${Sources})
target_link_libraries(visus PUBLIC VisusDb Python::Python)
set_target_properties(visus PROPERTIES FOLDER "Executable/")






