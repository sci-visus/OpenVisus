

# //////////////////////////////////////////////////////////////////////////
macro(ForceUnset name)
	unset(${name} CACHE)
	unset(${name})
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(ForceSet name value)
	ForceUnset(${name})
	set(${name} ${value)
endmacro()

# ///////////////////////////////////////////////
macro(SetIfNotDefined name value)
	if (NOT DEFINED ${name})
		set(${name} ${value})
	endif()
endmacro()


# /////////////////////////////////////////////////////////////
macro(DisableTargetWarnings Name)
	if(WIN32)
		target_compile_options(${Name} PRIVATE "/W0")
	else()
		target_compile_options(${Name} PRIVATE "-w")
	endif()
endmacro()

# /////////////////////////////////////////////////////////////
macro(DisableIncrementalLinking Name)
	if (WIN32)
		foreach (CONFIG DEBUG RELEASE RELWITHDEBINFO MINRELSIZE)
			set_property(TARGET ${Name} APPEND_STRING PROPERTY LINK_FLAGS_${CONFIG} " /INCREMENTAL:NO") #useless incremental for swig 
		endforeach()
	endif()
endmacro()

# ///////////////////////////////////////////////////
macro(SetVisibilityHidden Name)
	# this fixes the problem of static symbols conflicting with dynamic lib
	# example: dynamic libcrypto conflicting with internal static libcrypto
	if (NOT WIN32)
		target_compile_options(${Name} PRIVATE -fvisibility=hidden)
	endif()
endmacro()

# ///////////////////////////////////////////////////
macro(InstallDirectoryIfExists src dst) 
	if (EXISTS "${src}")
		install(DIRECTORY "${src}" DESTINATION ${dst})
	endif()
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(SetupCommonTargetOptions Name)
	set_target_properties(${Name} PROPERTIES CXX_STANDARD 11)
	set_target_properties(${Name} PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
	if (CMAKE_CONFIGURATION_TYPES)
		target_compile_options(${Name} PRIVATE "$<$<CONFIG:Debug>:-DVISUS_DEBUG=1>")	
	else()
		if (CMAKE_BUILD_TYPE STREQUAL "Debug")
			target_compile_options(${Name} PRIVATE -DVISUS_DEBUG=1)
		endif()
	endif()
	if (WIN32)
		target_compile_options(${Name} PRIVATE /bigobj)		
		target_compile_options(${Name} PRIVATE -DPSAPI_VERSION=1)
		target_compile_options(${Name} PRIVATE -DFD_SETSIZE=4096)
		target_compile_options(${Name} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
		target_compile_options(${Name} PRIVATE -DWIN32_LEAN_AND_MEAN)
	elseif (APPLE)
		target_compile_options(${Name} PRIVATE -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
		set_target_properties(${Name} PROPERTIES MACOSX_BUNDLE TRUE) 
		set_target_properties(${Name} PROPERTIES MACOSX_RPATH 0) # disable rpath
	else()
		target_compile_options(${Name} PRIVATE -D_FILE_OFFSET_BITS=64)
		target_compile_options(${Name} PRIVATE -Wno-attributes)	
	endif()
endmacro()

# ///////////////////////////////////////////////////
macro(Win32DisablePythonDebugVersion)
	list(LENGTH PYTHON_LIBRARY __n__)
	if (${__n__} EQUAL 1)
		set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_IMPLIB ${PYTHON_LIBRARY})
	else()
		list(FIND PYTHON_LIBRARY optimized __index__)
		math(EXPR __next_index__ "${__index__}+1")
		list(GET PYTHON_LIBRARY ${__next_index__} __release_library_location_)
		ForceUnset(PYTHON_LIBRARY)
		set(PYTHON_LIBRARY ${__release_library_location_})
		set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_IMPLIB ${__release_library_location_})
	endif()
	set_target_properties(OpenVisus::Python PROPERTIES INTERFACE_COMPILE_DEFINITIONS SWIG_PYTHON_INTERPRETER_NO_DEBUG=1)
endmacro()

# ///////////////////////////////////////////////////
macro(FindPythonLibrary)
	SetIfNotDefined(PYTHON_VERSION 3)
	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)	
	add_library(OpenVisus::Python SHARED IMPORTED GLOBAL)
	set_property(TARGET OpenVisus::Python APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	
	# disable Python debug library (most of pip packages do not work in debug mode)
	if (WIN32)
		Win32DisablePythonDebugVersion()
	else()
		set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 
	endif()
	message(STATUS "PYTHON_EXECUTABLE   ${PYTHON_EXECUTABLE}")
	message(STATUS "PYTHON_LIBRARY      ${PYTHON_LIBRARY}")
	message(STATUS "PYTHON_INCLUDE_DIR  ${PYTHON_INCLUDE_DIR}")
endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToLibrary Name)
	if (VISUS_PYTHON)
		if (WIN32)
			target_link_libraries(${Name} PUBLIC OpenVisus::Python)
		else()
			# for apple and linux I'm linking python only for Executables (or final shared dll such as  mod_visus) 
			# otherwise I'm going to have multiple libpython in the same process
			# with the error message: PyThreadState_Get: no current thread
			target_include_directories(${Name} PUBLIC ${PYTHON_INCLUDE_DIRS})
			if (APPLE)
				set_target_properties(${Name} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup") 	
			else()
				set_target_properties(${Name} PROPERTIES LINK_FLAGS "-Wl,--unresolved-symbols=ignore-all")
			endif()
		endif()
	endif()		
endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToExecutable Name)
	if (VISUS_PYTHON)
		if (WIN32)
			target_link_libraries(${Name} PUBLIC OpenVisus::Python)
		elseif (APPLE)
			target_link_libraries(${Name} PUBLIC OpenVisus::Python)
		else()
			# for nix is trickier since the linking order is important
			# the "-Wl,--start-group" does not always work (for example in travis)
			# see http://cmake.3232098.n2.nabble.com/Link-order-Ubuntu-tt7598592.html
			# i found this trick: VisusKernel should appear always before python 
			target_link_libraries(${Name} PUBLIC $<TARGET_FILE:VisusKernel> OpenVisus::Python)
		endif()
	endif()
endmacro()

# ///////////////////////////////////////////////////
macro(AddExternalLibrary Name)
	add_library(${Name} STATIC ${ARGN})
	SetupCommonTargetOptions(${Name})
	set_target_properties(${Name} PROPERTIES  ARCHIVE_OUTPUT_DIRECTORY ${InstallDir}/../ExternalLibs)
	SetVisibilityHidden(${Name})
	DisableTargetWarnings(${Name})
	set_target_properties(${Name} PROPERTIES FOLDER "ExternalLibs/")
endmacro()

# ///////////////////////////////////////////////////
macro(AddLibrary Name LibraryType)
	add_library(${Name} ${LibraryType} ${ARGN})
	LinkPythonToLibrary(${Name})
	SetupCommonTargetOptions(${Name})
	set_target_properties(${Name} PROPERTIES 
		LIBRARY_OUTPUT_DIRECTORY     ${InstallDir}/bin
		RUNTIME_OUTPUT_DIRECTORY     ${InstallDir}/bin
		ARCHIVE_OUTPUT_DIRECTORY     ${InstallDir}/lib) 
endmacro()

# ///////////////////////////////////////////////////
macro(AddExecutable Name)
	add_executable(${Name} ${ARGN})
	LinkPythonToExecutable(${Name})
	SetupCommonTargetOptions(${Name})
	set_target_properties(${Name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${InstallDir}/bin) 		
	set_target_properties(${Name} PROPERTIES FOLDER "Executable/")
endmacro()



# ///////////////////////////////////////////////////
macro(AddSwigLibrary Name CppName SwigFile)
	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})
	if (CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/OpenVisus) # this is for *.py generated files
		set(SWIG_OUTFILE_DIR  ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/swig) # this is for generated C++ and header files
	else()
		set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus)
		set(SWIG_OUTFILE_DIR  ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/swig)
	endif()
	set(CMAKE_SWIG_FLAGS "")
	set(SWIG_FLAGS "-threads;-extranative;${ARGN}")
	set_property(SOURCE ${SwigFile} PROPERTY SWIG_MODULE_NAME ${Name})
	set_source_files_properties(${SwigFile} PROPERTIES CPLUSPLUS ON)
	set_source_files_properties(${SwigFile} PROPERTIES SWIG_FLAGS "${SWIG_FLAGS}")
	if (CMAKE_VERSION VERSION_LESS "3.8")
		swig_add_module(${Name} python ${SwigFile})
	else()
		swig_add_library(${Name} LANGUAGE python SOURCES ${SwigFile})
	endif()
	if (TARGET _${Name})
		set(PyName _${Name})
	else()
		set(PyName ${Name})
	endif()
	target_compile_definitions(${PyName} PRIVATE SWIG_TYPE_TABLE=OpenVisus)
	target_compile_definitions(${PyName} PRIVATE VISUS_PYTHON=1)
	LinkPythonToLibrary(${PyName})
	DisableTargetWarnings(${PyName})
	SetupCommonTargetOptions(${PyName})
	DisableIncrementalLinking(${PyName})
	set_target_properties(${PyName} PROPERTIES 
		LIBRARY_OUTPUT_DIRECTORY     ${InstallDir}
		RUNTIME_OUTPUT_DIRECTORY     ${InstallDir}
		ARCHIVE_OUTPUT_DIRECTORY     ${InstallDir}/../swig) 
	target_link_libraries(${PyName} PUBLIC ${CppName})
	set_target_properties(${PyName} PROPERTIES FOLDER Swig/)
endmacro()

