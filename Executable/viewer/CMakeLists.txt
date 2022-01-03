
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

FILE(GLOB Sources *.cpp *.h)

add_executable(visusviewer ${Sources})

target_link_libraries(visusviewer PUBLIC VisusGui)
set_target_properties(visusviewer PROPERTIES FOLDER "Executable/")

if (VISUS_PYTHON)
	target_compile_definitions(visusviewer PUBLIC VISUS_PYTHON=1)
	if (WIN32)
		target_include_directories(visusviewer PUBLIC ${Python_INCLUDE_DIRS})
		# I know this sounds crazy, but if I don;t link both python3 and python3<N> I am having problems with `import PyQ5.QtGui`
		# linking both seems to solve the problem
		# example: C:/Python38/libs/python38.lib -> C:/Python38/libs/python3.lib
		target_link_libraries(visusviewer      PUBLIC ${Python_LIBRARY_RELEASE}/../python3.lib)
		target_link_libraries(visusviewer      PUBLIC ${Python_LIBRARY_RELEASE})
	else()
		target_link_libraries(visusviewer PUBLIC Python::Python) # Python library for Python embedding
	endif()
endif()

if (CLANG)
	set_target_properties(visusviewer PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
endif()
 
 