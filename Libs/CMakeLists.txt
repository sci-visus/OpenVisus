

# ///////////////////////////////////////////////////
macro(AddSwigLibrary NamePy WrappedLib SwigFile)

	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})

	if (CMAKE_CONFIGURATION_TYPES)
		set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}) # this is for generated C++ and header files
	else()
		set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	endif()

	set(CMAKE_SWIG_OUTDIR ${SWIG_OUTFILE_DIR}/OpenVisus) # this is for *.py generated files

	set(CMAKE_SWIG_FLAGS "")
	set(SWIG_FLAGS "${ARGN}")
	set(SWIG_FLAGS "${SWIG_FLAGS};-threads")
	set(SWIG_FLAGS "${SWIG_FLAGS};-extranative")

	#prevents rebuild every time make is called
	set_property(SOURCE ${SwigFile} PROPERTY SWIG_MODULE_NAME ${NamePy})

	set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
	set_source_files_properties(${SwigFile} PROPERTIES SWIG_FLAGS  "${SWIG_FLAGS}")

	if (CMAKE_VERSION VERSION_LESS "3.8")
		swig_add_module(${NamePy} python ${SwigFile})
	else()
		swig_add_library(${NamePy} LANGUAGE python SOURCES ${SwigFile})
	endif()
		
	if (TARGET _${NamePy})
		set(Name _${NamePy})
	else()
		set(Name ${NamePy})
	endif()

	target_compile_definitions(${Name}  PRIVATE SWIG_TYPE_TABLE=OpenVisus)

	target_link_libraries(${Name} PUBLIC Python::Module)

	# disable warnings
	if (WIN32)
		target_compile_definitions(${Name} PRIVATE /W0)
	else()
		set_target_properties(${Name} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()
	
	# I have the problem that
	# the swig generated *.py file and *.so must be in the same OpenVisus/ root directory
	# otherwise it won't work (since swig auto-generate "from . import _VisusKernelPy")     )

	if (CMAKE_CONFIGURATION_TYPES)
		set_target_properties(${Name} PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/swig) 		
	else()
		set_target_properties(${Name} PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/swig) 
	endif()

	target_link_libraries(${Name} PUBLIC ${WrappedLib})
	set_target_properties(${Name} PROPERTIES FOLDER Swig/)

endmacro()


add_subdirectory(Kernel)

add_subdirectory(Db)
add_subdirectory(XIdx)
add_subdirectory(Dataflow)
add_subdirectory(Nodes)

if (VISUS_GUI)
	add_subdirectory(Gui)
	add_subdirectory(GuiNodes)
	add_subdirectory(AppKit)
endif()
  
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Slam/CMakeLists.txt)
	message(STATUS "Enabled VisusSlam")
	add_subdirectory(Slam)
endif()

