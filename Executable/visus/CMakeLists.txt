
FILE(GLOB Sources *.h *.cpp)
AddExecutable(visus ${Sources})
target_link_libraries(visus PUBLIC VisusIdx)

if (NOT WIN32)
  target_link_libraries(visus PUBLIC OpenVisus::Python)
endif()




