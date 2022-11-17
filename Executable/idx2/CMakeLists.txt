FILE(GLOB Sources ReadMe.md *.h *.cpp)
add_executable(visus_idx2 ${Sources})

target_link_libraries(visus_idx2 PUBLIC VisusDb)
set_target_properties(visus_idx2 PROPERTIES FOLDER "Executable/")

add_executable(idx2App 
	${CMAKE_SOURCE_DIR}/Libs/IDX2/README.md 
	${CMAKE_SOURCE_DIR}/Libs/IDX2/Source/Applications/idx2App.cpp 
	${CMAKE_SOURCE_DIR}/Samples/python/idx2_convert.py)
target_link_libraries(idx2App PRIVATE idx2)
set_target_properties(idx2App PROPERTIES FOLDER "Executable/")





