
FILE(GLOB Sources *.h *.cpp)
AddExecutable(visus ${Sources})
target_link_libraries(visus PUBLIC VisusIdx)

# make sure all swig wrappers are built
if (TARGET VisusIdxPy)
  add_dependencies(visus VisusIdxPy VisusDbPy VisusDataflowPy VisusKernelPy)
else()
  add_dependencies(visus _VisusIdxPy _VisusDbPy _VisusDataflowPy _VisusKernelPy)
endif()

