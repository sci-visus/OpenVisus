
FILE(GLOB Sources *.h *.cpp ${CMAKE_SOURCE_DIR}/Samples/python/idx2_convert.py)
add_executable(visus ${Sources})

target_link_libraries(visus PUBLIC VisusDb)

if (VISUS_PYTHON)
	target_compile_definitions(visus PUBLIC VISUS_PYTHON=1)
	if (WIN32)
		target_include_directories(visus PUBLIC ${Python_INCLUDE_DIRS})
		target_link_libraries(visus      PUBLIC ${Python_LIBRARY_RELEASE})
	else()
		target_link_libraries(visus PUBLIC Python::Python) # Python library for Python embedding
	endif()
endif()

set_target_properties(visus PROPERTIES FOLDER "Executable/")






