
# /////////////////////////////////////////////////////////////////////
macro(AddSwigLibrary WrappedLib)

	set(NamePy   ${WrappedLib}Py)
	set(SwigFile ${CMAKE_CURRENT_SOURCE_DIR}/${NamePy}.i)

	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../XIdx/include")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Db/include")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Dataflow/include")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Nodes/include")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Gui/include")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../slamcpp")

	# this is for generated C++ and header files
	set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${IntConfigName}) 

	set(CMAKE_SWIG_OUTDIR ${SWIG_OUTFILE_DIR}/OpenVisus) # this is for *.py generated files
	set (UseSWIG_TARGET_NAME_PREFERENCE STANDARD)

	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-threads;-extranative")

	# important for good name generation id Debug mode (https://gitlab.kitware.com/cmake/cmake/issues/18771)
	# set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-interface;${NamePy}")

	# set_property(SOURCE ${SwigFile} PROPERTY SWIG_MODULE_NAME ${NamePy}) prevents rebuild every time make is called (? scrgiorgio sommented for now)
	set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
	
	swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile} ${ARGN})
	target_compile_definitions(${NamePy} PRIVATE SWIG_TYPE_TABLE=OpenVisus)

	target_link_libraries(${NamePy} PUBLIC ${WrappedLib})
	set_target_properties(${NamePy} PROPERTIES FOLDER Swig/)

	if (WIN32)
		target_compile_definitions(${NamePy} PRIVATE /W0) # disable warnings
	else()
		set_target_properties(${NamePy} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()

	# swig generated *.py file and *.so must be in the same OpenVisus/ root directoryc otherwise it won't work (since swig auto-generate "from . import _VisusKernelPy") 
	set_target_properties(${NamePy} PROPERTIES 
		LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME} 
		RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}
		ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/swig) 

endmacro()

if (VISUS_SLAM)
	set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/VisusKernelPy.i APPEND PROPERTY SWIG_FLAGS  "-DVISUS_SLAM=1")
endif()

AddSwigLibrary(VisusKernel __init__.py __main__.py setup.py utils.py VisusPy.common)
AddSwigLibrary(VisusXIdx)
AddSwigLibrary(VisusDataflow dataset.py)
AddSwigLibrary(VisusDb)
AddSwigLibrary(VisusNodes)
AddSwigLibrary(VisusGui scripting_node.py viewer.py gui.py)

if (VISUS_SLAM)
	target_compile_definitions(VisusKernelPy PRIVATE VISUS_SLAM=1)
	target_include_directories(VisusKernelPy PRIVATE  ../slamcpp)
endif()

CopyToBinaryDirectory(TARGET VisusKernelPy FILES  __init__.py            DESTINATION ./)
CopyToBinaryDirectory(TARGET VisusKernelPy FILES  __main__.py            DESTINATION ./)
CopyToBinaryDirectory(TARGET VisusKernelPy FILES  setup.py               DESTINATION ./)
CopyToBinaryDirectory(TARGET VisusKernelPy FILES  utils.py               DESTINATION ./)

CopyToBinaryDirectory(TARGET VisusDbPy     FILES  dataset.py             DESTINATION ./)

CopyToBinaryDirectory(TARGET VisusGuiPy    FILES  scripting_node.py      DESTINATION ./)
CopyToBinaryDirectory(TARGET VisusGuiPy    FILES  gui.py                 DESTINATION ./)
CopyToBinaryDirectory(TARGET VisusGuiPy    FILES  viewer.py              DESTINATION ./)