
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB Sources *.cpp *.h)
AddExecutable(visusviewer ${Sources})

target_link_libraries(visusviewer PUBLIC VisusAppKit)

if (NOT WIN32)
  target_link_libraries(visusviewer PUBLIC ${PYTHON_LIBRARY})
endif()

