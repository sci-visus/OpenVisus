find_package(SWIG 3.0 REQUIRED)
include(${SWIG_USE_FILE})	

# /////////////////////////////////////////////////////////////////////
macro(AddSwigLibrary WrappedLib)

	set(NamePy   ${WrappedLib}Py)
	set(SwigFile ${CMAKE_CURRENT_SOURCE_DIR}/${NamePy}.i)

	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}")
	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/../Kernel/include")
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

	set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
	
	swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile} ${ARGN})
	target_compile_definitions(${NamePy} PRIVATE SWIG_TYPE_TABLE=OpenVisus)
	target_compile_definitions(${NamePy} PRIVATE SWIG_PYTHON_INTERPRETER_NO_DEBUG=1)

	target_include_directories(${NamePy} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(${NamePy} PRIVATE ${WrappedLib})
	set_target_properties(${NamePy} PROPERTIES FOLDER Swig/)

	target_compile_definitions(${NamePy} PUBLIC VISUS_PYTHON=1)
	if (WIN32)
		target_include_directories(${NamePy} PUBLIC ${Python_INCLUDE_DIRS})
		target_link_libraries(${NamePy}      PUBLIC ${Python_LIBRARY_RELEASE})
	else()
		target_link_libraries(${NamePy} PUBLIC Python::Module) # Python library for Python extending
	endif()

	if (MSVC)
		target_compile_definitions(${NamePy} PRIVATE /W0) 
		target_compile_options(${NamePy}     PRIVATE /bigobj)
	else()
		set_target_properties(${NamePy} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()

	# swig generated *.py file and *.so must be in the same OpenVisus/ root directoryc otherwise it won't work (since swig auto-generate "from . import _VisusPy") 
	set_target_properties(${NamePy} PROPERTIES 
		LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME} 
		RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/${PROJECT_NAME}
		ARCHIVE_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${ConfigName}/swig) 

endmacro()

macro(AddPythonTarget GroupName Name)
	add_custom_target(${Name} ${CMAKE_COMMAND} 
		-E env PYTHONPATH=${CMAKE_BINARY_DIR}/${ConfigName} 
		${Python_EXECUTABLE} -u ${ARGN} 
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/${ConfigName}/OpenVisus/)
	set_property(TARGET ${Name} PROPERTY FOLDER ${GroupName})
endmacro()

if (VISUS_SLAM)
	set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/VisusPy.i APPEND PROPERTY SWIG_FLAGS  "-DVISUS_SLAM=1")
endif()

AddSwigLibrary(Visus __init__.py __main__.py setup.py utils.py image_utils.py VisusPy.common dataset.py PyMultipleDataset.h)

if (VISUS_SLAM)
	target_compile_definitions(VisusPy PRIVATE VISUS_SLAM=1)
	target_include_directories(VisusPy PRIVATE  ../slamcpp)
endif()

AddPythonTarget("CMakePredefinedTargets" test      -m OpenVisus test)
AddPythonTarget("CMakePredefinedTargets" test-idx  -m OpenVisus test-idx)


if (TARGET VisusGui)
	AddSwigLibrary(VisusGui scripting_node.py viewer.py gui.py)
	AddPythonTarget("CMakePredefinedTargets" configure          -m OpenVisus configure)
	AddPythonTarget("CMakePredefinedTargets" viewer             -m OpenVisus viewer)
	AddPythonTarget("CMakePredefinedTargets" test-dataflow      -m OpenVisus test-dataflow)
	AddPythonTarget("CMakePredefinedTargets" test-gui           -m OpenVisus test-gui)
	AddPythonTarget("CMakePredefinedTargets" test-viewer        -m OpenVisus test-viewer)
	AddPythonTarget("CMakePredefinedTargets" test-viewer1       -m OpenVisus test-viewer1)
	AddPythonTarget("CMakePredefinedTargets" test-viewer2       -m OpenVisus test-viewer2)	
	AddPythonTarget("CMakePredefinedTargets" test-two-viewers   -m OpenVisus test-two-viewers)
endif()


