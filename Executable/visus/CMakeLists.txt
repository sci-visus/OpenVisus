
FILE(GLOB Sources *.h *.cpp)
add_executable(visus ${Sources})
set_target_properties(visus PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set_target_properties(visus PROPERTIES FOLDER Executable/)
target_link_libraries(visus PUBLIC VisusIdx)

# make sure all swig wrappers are built
add_dependencies(visus _VisusIdxPy _VisusDbPy _VisusDataflowPy _VisusKernelPy)
