
file(GLOB Sources include/Visus/*.h src/*.cpp)
source_group("" FILES ${Sources})
AddLibrary(VisusIdx SHARED ${Sources})
target_link_libraries(VisusIdx PUBLIC VisusDb)


if (VISUS_PYTHON)
	AddSwigLibrary(VisusIdxPy VisusIdx ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus/VisusIdxPy.i 
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include"
		"-I${CMAKE_CURRENT_SOURCE_DIR}/include")
endif()


