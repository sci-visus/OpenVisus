

file(GLOB Sources *.cpp *.h *.py)
add_executable(python ${Sources})
set_target_properties(python PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

set_target_properties(python PROPERTIES FOLDER Executable/)

if (TARGET python27)
	target_link_libraries(python PUBLIC python27)
else()
	target_include_directories(python PUBLIC ${PYTHON_INCLUDE_DIRS})
	target_link_libraries(python PUBLIC ${PYTHON_LIBRARY})
endif()



