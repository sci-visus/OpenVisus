

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

# //////////////////////////////////////////////////////////////////////
macro(DownloadAndUncompress url check_exist working_directory)
	if (NOT EXISTS "${check_exist}")
		file(MAKE_DIRECTORY ${working_directory})
		message(STATUS "Downloading ${url}")
		get_filename_component(filename ${url} NAME)
		set(filename ${CMAKE_BINARY_DIR}/${filename})		
		file(DOWNLOAD ${url} ${filename})
		message(STATUS "Unzipping file ${filename} into ${working_directory}")
		file(MAKE_DIRECTORY ${check_exist})
		execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${filename} WORKING_DIRECTORY ${working_directory})
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

		# this seem to block windows 10
		# target_compile_options(${Name} PRIVATE /MP)

		target_compile_options(${Name} PRIVATE /bigobj)		
		# see http://msdn.microsoft.com/en-us/library/windows/desktop/ms683219(v=vs.85).aspx
		target_compile_options(${Name} PRIVATE -DPSAPI_VERSION=1)
		target_compile_options(${Name} PRIVATE -DFD_SETSIZE=4096)
		target_compile_options(${Name} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
		target_compile_options(${Name} PRIVATE -DWIN32_LEAN_AND_MEAN)		
		
	elseif (APPLE)
	
		# suppress some warnings
		target_compile_options(${Name} PRIVATE -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)

		set_target_properties(${Name} PROPERTIES MACOSX_BUNDLE TRUE) 
		set_target_properties(${Name} PROPERTIES MACOSX_RPATH 0) # disable rpath

	else()
	
		# enable 64 bit file support (see http://learn-from-the-guru.blogspot.it/2008/02/large-file-support-in-linux-for-cc.html)
		target_compile_options(${Name} PRIVATE -D_FILE_OFFSET_BITS=64)

		# -Wno-attributes to suppress spurious "type attributes ignored after type is already defined" messages 
		# see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39159
		target_compile_options(${Name} PRIVATE -Wno-attributes)	
	
	endif()

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
	else()
		set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 
	endif()

	message(STATUS "PYTHON_EXECUTABLE   ${PYTHON_EXECUTABLE}")
	message(STATUS "PYTHON_LIBRARY      ${PYTHON_LIBRARY}")
	message(STATUS "PYTHON_INCLUDE_DIR  ${PYTHON_INCLUDE_DIR}")
		
endmacro()


# ///////////////////////////////////////////////////
macro(AddExternalLibrary Name)

	add_library(${Name} STATIC ${ARGN})

	SetupCommonTargetOptions(${Name})

	# see https://stackoverflow.com/questions/56514533/how-to-specify-the-output-directory-of-a-given-dll
	if (CMAKE_CONFIGURATION_TYPES)
		set_target_properties(${Name} PROPERTIES  	
			ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/ExternalLib )
	else()
		set_target_properties(${Name} PROPERTIES	
			ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/ExternalLib ) 
	endif()

	# this fixes the problem of static symbols conflicting with dynamic lib
	# example: dynamic libcrypto conflicting with internal static libcrypto
	if (NOT WIN32)
		target_compile_options(${Name} PRIVATE -fvisibility=hidden)
	endif()

	DisableTargetWarnings(${Name})
	
	set_target_properties(${Name} PROPERTIES FOLDER "ExternalLibs/")

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
macro(AddLibrary Name LibraryType)

	add_library(${Name} ${LibraryType} ${ARGN})

	LinkPythonToLibrary(${Name})
	SetupCommonTargetOptions(${Name})

	if (CMAKE_CONFIGURATION_TYPES)
		set_target_properties(${Name} PROPERTIES 
			LIBRARY_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus/bin
			RUNTIME_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus/bin
			ARCHIVE_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus/lib) 		
	else()
		set_target_properties(${Name} PROPERTIES 
			LIBRARY_OUTPUT_DIRECTORY      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/bin
			RUNTIME_OUTPUT_DIRECTORY      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/bin	
			ARCHIVE_OUTPUT_DIRECTORY      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/lib) 
	endif()


	if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include)
		string(REPLACE "Visus" "" __tmp__ ${Name})
		install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Visus DESTINATION ${InstallDir}/include/${__tmp__}/)
	endif()

	string(TOUPPER ${Name} __NAME__)
	target_compile_definitions(${Name}  PRIVATE VISUS_BUILDING_${__NAME__}=1)
	target_include_directories(${Name}  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)

	# example VISUS_STATIC_KERNEL_LIB
	get_target_property(target_type ${Name} TYPE)
	if (target_type STREQUAL "STATIC_LIBRARY")
		string(TOUPPER ${__NAME__} __definition__)
		set(__definition__ "VISUS_STATIC_${__definition__}_LIB=1")
		string(REPLACE "VISUS_STATIC_VISUS" "VISUS_STATIC_" __definition__ "${__definition__}")
		target_compile_definitions(${Name}  PUBLIC ${__definition__})
	endif ()	

	# example: VisusKernel -> VISUS_KERNEL
	string(REPLACE "VISUS" "" __out__ ${__NAME__})
	target_compile_definitions(${Name}  PUBLIC VISUS_${__out__}=1) 

	set_target_properties(${Name} PROPERTIES FOLDER "")

endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToExecutable Name)

	if (VISUS_PYTHON)
		if (WIN32)
			# already linked
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
macro(AddExecutable Name)
	add_executable(${Name} ${ARGN})

	LinkPythonToExecutable(${Name})
	SetupCommonTargetOptions(${Name})

	if (CMAKE_CONFIGURATION_TYPES)
		set_target_properties(${Name} PROPERTIES  
			RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus/bin) 		
	else()
		set_target_properties(${Name} PROPERTIES
			RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/bin) 
	endif()

	set_target_properties(${Name} PROPERTIES FOLDER "Executable/")
endmacro()



# ///////////////////////////////////////////////////
macro(AddSwigLibrary NamePy WrappedLib SwigFile)

	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})
	
	# this is for *.py generated files
	if (CMAKE_CONFIGURATION_TYPES)
		set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/OpenVisus)
	else()
		set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus)
	endif()
	
	# this is for generated C++ and header files
	if (CMAKE_CONFIGURATION_TYPES)
		set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})
	else()
		set(SWIG_OUTFILE_DIR ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	
	endif()
	
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
	target_compile_definitions(${Name}  PRIVATE VISUS_PYTHON=1)

	LinkPythonToLibrary(${Name})
	
	# disable warnings
	if (WIN32)
		target_compile_definitions(${Name}  PRIVATE /W0)
	else()
		set_target_properties(${Name} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()
	
	SetupCommonTargetOptions(${Name})
		
	# I have the problem that
	# the swig generated *.py file and *.so must be in the same OpenVisus/ root directory
	# otherwise it won't work (since swig auto-generate "from . import _VisusKernelPy")     )

	if (CMAKE_CONFIGURATION_TYPES)
		set_target_properties(${Name} PROPERTIES 
			LIBRARY_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus
			RUNTIME_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/$<CONFIG>/OpenVisus 
			ARCHIVE_OUTPUT_DIRECTORY     ${CMAKE_BINARY_DIR}/$<CONFIG>/swig) 		
	else()
		set_target_properties(${Name} PROPERTIES 
			LIBRARY_OUTPUT_DIRECTORY      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus 
			RUNTIME_OUTPUT_DIRECTORY      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus
			ARCHIVE_OUTPUT_DIRECTORY      ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/swig) 
	endif()

		
	target_link_libraries(${Name} PUBLIC ${WrappedLib})
	set_target_properties(${Name} PROPERTIES FOLDER Swig/)
	DisableIncrementalLinking(${Name})
	# set_property(TARGET ${Name} APPEND_STRING PROPERTY LINK_FLAGS_${CONFIG} " /pdb:none") # do I need it for debugging?

	
endmacro()


# ///////////////////////////////////////////////////
macro(GenerateScript template_filename script_filename target_filename)

	if (WIN32)
		set(SCRIPT_EXT ".bat")
	elseif (APPLE)	
		set(SCRIPT_EXT ".command")
	else()
		set(SCRIPT_EXT ".sh")
	endif()

	file(READ "${template_filename}${SCRIPT_EXT}" content) 
	
	string(REPLACE "\${PYTHON_EXECUTABLE}" "${PYTHON_EXECUTABLE}" content "${content}")
	if (VISUS_GUI)
		string(REPLACE "\${VISUS_GUI}" "1" content "${content}")
	else()
		string(REPLACE "\${VISUS_GUI}" "0" content "${content}")
	endif()
	
	if (WIN32)
		string(REPLACE "\${TARGET_FILENAME}" "${target_filename}.exe" content  "${content}")
	
	elseif (APPLE)
		get_filename_component(__name_we__ ${target_filename} NAME_WE)
		string(REPLACE "\${TARGET_FILENAME}" "${target_filename}.app/Contents/MacOS/${__name_we__}" content  "${content}")
	else()
		string(REPLACE "\${TARGET_FILENAME}" "${target_filename}" content  "${content}")
	endif()
	
	file(GENERATE OUTPUT "${script_filename}${SCRIPT_EXT}" CONTENT "${content}")
endmacro()

# ///////////////////////////////////////////////////
macro(InstallDirectoryIfExists src dst) 
	if (EXISTS "${src}")
		install(DIRECTORY "${src}" DESTINATION ${dst})
	endif()
endmacro()


# ///////////////////////////////////////////////////////////////
macro(GitUpdateSubmodules)
	find_package(Git REQUIRED)
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} RESULT_VARIABLE __result__)
        if(NOT __result__ EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${__result__}, please checkout submodules")
        endif()
endmacro()


