find_package(SWIG 3.0 REQUIRED)
include(${SWIG_USE_FILE})	

# /////////////////////////////////////////////////////////////////////
macro(AddSwigLibrary WrappedLib)

	set(NamePy   ${WrappedLib}Py)
	set(SwigFile ${CMAKE_CURRENT_SOURCE_DIR}/${NamePy}.i)

	set_property(SOURCE ${SwigFile} APPEND PROPERTY SWIG_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}")
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

	set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
	
	swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile} ${ARGN})
	target_compile_definitions(${NamePy} PRIVATE SWIG_TYPE_TABLE=OpenVisus)
	target_compile_definitions(${NamePy} PRIVATE SWIG_PYTHON_INTERPRETER_NO_DEBUG=1)

	target_include_directories(${NamePy} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(${NamePy} PRIVATE ${WrappedLib})
	set_target_properties(${NamePy} PROPERTIES FOLDER Swig/)

	if (VISUS_PYTHON)
		target_compile_definitions(${NamePy} PUBLIC VISUS_PYTHON=1)
		if (WIN32)
			target_include_directories(${NamePy} PUBLIC ${Python_INCLUDE_DIRS})
			target_link_libraries(${NamePy}      PUBLIC ${Python_LIBRARY_RELEASE})
		else()
			target_link_libraries(${NamePy} PUBLIC Python::Module) # Python library for Python extending
		endif()
	endif()

	if (WIN32)
		target_compile_definitions(${NamePy} PRIVATE /W0) 
		target_compile_options(${NamePy} PRIVATE /bigobj)
	else()
		set_target_properties(${NamePy} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()

	# swig generated *.py file and *.so must be in the same OpenVisus/ root directoryc otherwise it won't work (since swig auto-generate "from . import _VisusKernelPy") 
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

if (TARGET VisusKernel)

	if (VISUS_SLAM)
		set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/VisusKernelPy.i APPEND PROPERTY SWIG_FLAGS  "-DVISUS_SLAM=1")
	endif()

	AddSwigLibrary(VisusKernel __init__.py __main__.py setup.py utils.py VisusPy.common)

	if (VISUS_SLAM)
		target_compile_definitions(VisusKernelPy PRIVATE VISUS_SLAM=1)
		target_include_directories(VisusKernelPy PRIVATE  ../slamcpp)
	endif()

endif()

if (TARGET VisusXIdx)
	AddSwigLibrary(VisusXIdx)
endif()

if (TARGET VisusDb)
	AddSwigLibrary(VisusDb dataset.py PyMultipleDataset.h)
endif()

if (TARGET VisusDataflow)
	AddSwigLibrary(VisusDataflow)
	AddPythonTarget("CMakePredefinedTargets" test -m OpenVisus test)
endif()

if (TARGET VisusNodes)
	AddSwigLibrary(VisusNodes)
endif()

if (TARGET VisusGui)
	AddSwigLibrary(VisusGui scripting_node.py viewer.py gui.py)
	AddPythonTarget("CMakePredefinedTargets" configure -m OpenVisus configure)
	AddPythonTarget("CMakePredefinedTargets" viewer    -m OpenVisus viewer)
	AddPythonTarget("CMakePredefinedTargets" viewer1   -m OpenVisus viewer1)
	AddPythonTarget("CMakePredefinedTargets" viewer2   -m OpenVisus viewer2)	

	AddPythonTarget("Jupyter" quick_tour.ipynb     -m OpenVisus jupyter quick_tour.ipynb)
	AddPythonTarget("Jupyter" Agricolture.ipynb    -m OpenVisus jupyter Samples/jupyter/Agricolture.ipynb)
	AddPythonTarget("Jupyter" Climate.ipynb        -m OpenVisus jupyter Samples/jupyter/Climate.ipynb)
	AddPythonTarget("Jupyter" ReadAndView.ipynb    -m OpenVisus jupyter Samples/jupyter/ReadAndView.ipynb)
endif()


