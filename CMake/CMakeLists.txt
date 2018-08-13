

# post install code
if (VISUS_GUI)
	if (WIN32)
		install(CODE "
			if(\${BUILD_TYPE} STREQUAL \"Debug\")
				execute_process(COMMAND ${WINDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/debug/bin/visusviewer.exe\" WORKING_DIRECTORY ${QT5_BINARY_DIR})
			else()
				execute_process(COMMAND ${WINDEPLOYQT} \"\${CMAKE_INSTALL_PREFIX}/bin/visusviewer.exe\" WORKING_DIRECTORY ${QT5_BINARY_DIR})
			endif()
		")	
	endif()
endif()