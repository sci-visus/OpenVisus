
FILE(GLOB Sources *.h *.cpp)
AddVisusExecutable(visus ${Sources})
target_link_libraries(visus PUBLIC VisusIdx)

# make sure all swig wrappers are built
add_dependencies(visus _VisusIdxPy _VisusDbPy _VisusDataflowPy _VisusKernelPy)
