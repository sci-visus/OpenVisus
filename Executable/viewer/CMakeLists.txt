
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB Sources *.cpp *.h)
AddExecutable(visusviewer ${Sources})
target_link_libraries(visusviewer PUBLIC VisusAppKit)

if (APPLE)
	set_target_properties(visusviewer PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif()
 
 