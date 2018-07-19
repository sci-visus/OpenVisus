

file(GLOB Sources *.cpp *.h *.py)
AddExecutable(python ${Sources})

if (TARGET python27)
	target_link_libraries(python PUBLIC python27)
else()
	target_include_directories(python PUBLIC ${PYTHON_INCLUDE_DIRS})
	target_link_libraries(python PUBLIC ${PYTHON_LIBRARY})
endif()



