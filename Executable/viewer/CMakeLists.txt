

set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB Sources *.cpp *.h)
add_executable(visusviewer ${Sources})

target_link_libraries(visusviewer PUBLIC VisusGui)

if (VISUS_PYTHON AND NOT WIN32)
	target_link_libraries(visusviewer PUBLIC Python::Python)
endif()

set_target_properties(visusviewer PROPERTIES FOLDER "Executable/")

if (APPLE)
	set_target_properties(visusviewer PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif()
 
 