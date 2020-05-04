
FILE(GLOB Sources *.h *.cpp)
add_executable(visus ${Sources})

target_link_libraries(visus PUBLIC VisusDb)

if (NOT WIN32)
	target_link_libraries(visus PUBLIC Python::Python)
endif()

set_target_properties(visus PROPERTIES FOLDER "Executable/")






