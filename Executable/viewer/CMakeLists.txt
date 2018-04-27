
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB Sources *.cpp *.h)
add_executable(visusviewer ${Sources})
set_target_properties(visusviewer PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})
set_target_properties(visusviewer PROPERTIES FOLDER Executable/)
target_link_libraries(visusviewer PUBLIC VisusAppKit)

# make sure all swig wrappers are built
add_dependencies(visusviewer
  _VisusAppKitPy _VisusGuiNodesPy _VisusGuiPy
  _VisusNodesPy _VisusIdxPy _VisusDbPy _VisusDataflowPy _VisusKernelPy)
