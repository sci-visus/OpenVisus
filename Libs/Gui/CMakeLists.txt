




set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

file(GLOB Sources include/Visus/*.h src/*.cpp resources/*.glsl resources/*.qrc)

source_group("" FILES ${Sources})

AddLibrary(VisusGui SHARED ${Sources})

target_link_libraries(VisusGui PUBLIC VisusKernel)
target_link_libraries(VisusGui PUBLIC Qt5::Core Qt5::Widgets Qt5::Gui Qt5::OpenGL)

if (WIN32)
	target_link_libraries(VisusGui PUBLIC opengl32.lib glu32.lib)
elseif (APPLE)
	target_link_libraries(VisusGui PUBLIC "-framework OpenGL" "-framework AGL")
else() 
	target_link_libraries(VisusGui PUBLIC GL GLU X11)
endif()


if (VISUS_PYTHON)
	AddSwigLibrary(VisusGui ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusGuiPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")

endif()
  
