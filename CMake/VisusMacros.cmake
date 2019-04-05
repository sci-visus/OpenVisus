

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

# //////////////////////////////////////////////////////////////////////////
macro(DetectOsxVersion)

	if (NOT CMAKE_C_COMPILER)
		MESSAGE(FATAL "Empty CMAKE_C_COMPILER, do you have a compiler installed?")
	endif()

	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE)
	string(REGEX REPLACE ".*-darwin([0-9]+).*" "\\1" _apple_ver "${MACHINE}")
	
	if (_apple_ver EQUAL "18")
		set(APPLE_OSX_VERSION 10.14)
	elseif (_apple_ver EQUAL "17")
		set(APPLE_OSX_VERSION 10.13)
	elseif (_apple_ver EQUAL "16")
		set(APPLE_OSX_VERSION 10.12)
	elseif (_apple_ver EQUAL "15")
		set(APPLE_OSX_VERSION 10_11)
	elseif (_apple_ver EQUAL "14")
		set(APPLE_OSX_VERSION 10_10)
	elseif (_apple_ver EQUAL "13")
		set(APPLE_OSX_VERSION 10_9)
	elseif (_apple_ver EQUAL "12")
		set(APPLE_OSX_VERSION 10_8)
	elseif (_apple_ver EQUAL "11")
		set(APPLE_OSX_VERSION 10_7)
	elseif (_apple_ver EQUAL "10")
		set(APPLE_OSX_VERSION 10_6)
	else()
		message(FATAL "Unknown osx version ${APPLE_OSX_VERSION}")
	endif()
	message(STATUS "APPLE_OSX_VERSION ${APPLE_OSX_VERSION}") 

endmacro()


# //////////////////////////////////////////////////////////////////////////
macro(SetupCommonCMake)

	if (NOT __SETUP_COMMON_CMAKE__)
	
		set(__SETUP_COMMON_CMAKE__ 1)
	
		set(CMAKE_CXX_STANDARD 11)
		set(CMAKE_POSITION_INDEPENDENT_CODE ON)
		set_property(GLOBAL PROPERTY USE_FOLDERS ON)  
		set(CMAKE_NUM_PROCS 8)   
		
		option(BUILD_SHARED_LIBS "Build the shared library" TRUE)
		
		# multi-config generator
		if (CMAKE_CONFIGURATION_TYPES)
		
			message(STATUS "Cmake is using multi-config generator (${CMAKE_CONFIGURATION_TYPES})")	
			add_compile_options("$<$<CONFIG:Debug>:-DVISUS_DEBUG=1>")	
			
		else()

			message(STATUS "Cmake is using single-config generator")
			
			if(NOT CMAKE_BUILD_TYPE)
			  set(CMAKE_BUILD_TYPE "Release")
			endif()
			 
			if (CMAKE_BUILD_TYPE EQUAL "Debug")
				add_compile_options("$<$<CONFIG:Debug>:-DVISUS_DEBUG=1>")	
			endif()
			
		endif()	
		
		if (WIN32)
			if (CMAKE_TOOLCHAIN_FILE)
				set(VCPKG 1)
			else()
				set(VCPKG 0)
			endif()	
			
			# on windows I'm always using the Release version because
			# (1) numpy is not available in debug mode
			# (2) lot of problems/incompatibilities happens (for example: https://github.com/swig/swig/issues/1321)			
			SET(SWIG_PYTHON_INTERPRETER_NO_DEBUG "1" CACHE INTERNAL "")
			MESSAGE(STATUS "Forcing SWIG_PYTHON_INTERPRETER_NO_DEBUG for windows")
			
		endif()		
		
		if (APPLE)
			DetectOsxVersion()
			set(CMAKE_MACOSX_BUNDLE YES)
			set(CMAKE_MACOSX_RPATH  0)	 # disable rpath
		endif()
		
		if (WIN32)
			set(SCRIPT_EXTENSION ".bat")
		elseif (APPLE)
			set(SCRIPT_EXTENSION ".command")
		else()
			set(SCRIPT_EXTENSION ".sh")
		endif()	
		
	endif()

	if (CMAKE_CONFIGURATION_TYPES)
		set(PYTHON_BINARY_DIR ${CMAKE_BINARY_DIR}/$<CONFIG>/site-packages/${CMAKE_PROJECT_NAME})
	else()
		set(PYTHON_BINARY_DIR ${CMAKE_BINARY_DIR}/site-packages/${CMAKE_PROJECT_NAME})
	endif()

	set(SWIG_TYPE_TABLE ${CMAKE_PROJECT_NAME})
	
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(SetupCommonCompileOptions Name)

	if (WIN32)

		target_compile_options(${Name} PRIVATE /MP)
		target_compile_options(${Name} PRIVATE /bigobj)		
		# see http://msdn.microsoft.com/en-us/library/windows/desktop/ms683219(v=vs.85).aspx
		target_compile_options(${Name} PRIVATE -DPSAPI_VERSION=1)
		target_compile_options(${Name} PRIVATE -DFD_SETSIZE=4096)
		target_compile_options(${Name} PRIVATE -D_CRT_SECURE_NO_WARNINGS)
		target_compile_options(${Name} PRIVATE -DWIN32_LEAN_AND_MEAN)		
		
	elseif (APPLE)
	
		# suppress some warnings
		target_compile_options(${Name} PRIVATE  -Wno-unused-variable -Wno-unused-parameter -Wno-reorder)
	
	else()
	
		# enable 64 bit file support (see http://learn-from-the-guru.blogspot.it/2008/02/large-file-support-in-linux-for-cc.html)
		target_compile_options(${Name} PRIVATE -D_FILE_OFFSET_BITS=64)
		# -Wno-attributes to suppress spurious "type attributes ignored after type is already defined" messages 
		# see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39159
		target_compile_options(${Name} PRIVATE -Wno-attributes)	
	
	endif()

endmacro()

# ///////////////////////////////////////////////////
macro(SetupCommonOutputTargetProperties Name)

	set_target_properties(${Name} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${PYTHON_BINARY_DIR}/bin) 
	set_target_properties(${Name} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PYTHON_BINARY_DIR}/lib) 
	set_target_properties(${Name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PYTHON_BINARY_DIR}/bin) 

	if (WIN32)
		set_target_properties(${Name} PROPERTIES COMPILE_PDB_NAME ${Name})
	endif()

endmacro()


# //////////////////////////////////////////////////////////////////////////
macro(FindOpenMP)
	if (DISABLE_OPENMP)
		MESSAGE(STATUS "OpenMP disabled")	
	else()
		if (APPLE)
			# FindOpenMP.cmake seems broken, at least for me
			# see https://iscinumpy.gitlab.io/post/omp-on-high-sierra/
			set(OpenMP_INCLUDE_DIR /usr/local/opt/libomp/include)
			set(OpenMP_LIBRARY     /usr/local/opt/libomp/lib/libomp.a)
			if(EXISTS "${OpenMP_INCLUDE_DIR}/omp.h" AND EXISTS ${OpenMP_LIBRARY})
				set(OpenMP_FOUND 1)
				include_directories(${OpenMP_INCLUDE_DIR})
				link_libraries(${OpenMP_LIBRARY})
				set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}    -Xpreprocessor -fopenmp")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Xpreprocessor -fopenmp")
			else()
				set(OpenMP_FOUND 0)
			endif()
		else()
			find_package(OpenMP)
			if (OpenMP_FOUND)
			 	set(CMAKE_C_FLAGS          "${CMAKE_C_FLAGS}          ${OpenMP_C_FLAGS}")
				set(CMAKE_CXX_FLAGS        "${CMAKE_CXX_FLAGS}        ${OpenMP_CXX_FLAGS}")
				set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
			endif()				
		endif()
		if (OpenMP_FOUND)
			MESSAGE(STATUS "Found OpenMP")	
			if (WIN32)
				set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
			endif()	
		else()
			MESSAGE(STATUS "OpenMP not found")
		endif()	
	endif()
endmacro()

# /////////////////////////////////////////////////////////////
macro(DisableAllWarnings)
	set(CMAKE_C_WARNING_LEVEL   0)
	set(CMAKE_CXX_WARNING_LEVEL 0)
	if(WIN32)
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   /W0")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W0")
	else()
		set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -w")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")
	endif()
endmacro()

# ///////////////////////////////////////////////////
macro(FindPythonLibrary)

	SetIfNotDefined(PYTHON_VERSION 3)

	find_package(PythonInterp ${PYTHON_VERSION} REQUIRED)
	find_package(PythonLibs   ${PYTHON_VERSION} REQUIRED)	

	message(STATUS "PYTHON_EXECUTABLE   ${PYTHON_EXECUTABLE}")
	message(STATUS "PYTHON_LIBRARY      ${PYTHON_LIBRARY}")
	message(STATUS "PYTHON_INCLUDE_DIR  ${PYTHON_INCLUDE_DIR}")
		
	add_library(Imported::Python SHARED IMPORTED GLOBAL)
	set_property(TARGET Imported::Python APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	
		
	if (WIN32)
		list(LENGTH PYTHON_LIBRARY __n__)
		if (${__n__} EQUAL 1)
			set(PYTHON_DEBUG_LIBRARY     ${PYTHON_LIBRARY})
			set(PYTHON_RELEASE_LIBRARY   ${PYTHON_LIBRARY})
		else()
			# differentiate debug from release
			# example debug;aaaa;optimized;bbb

			list(FIND PYTHON_LIBRARY optimized __index__)
		    if (${__index__} EQUAL -1)
		    	MESSAGE(ERROR "Problem with find python")
		    endif()
		    math(EXPR __next_index__ "${__index__}+1")
		    list(GET PYTHON_LIBRARY ${__next_index__} PYTHON_RELEASE_LIBRARY)

			list(FIND PYTHON_LIBRARY debug __index__)
			if (${__index__} EQUAL -1)
		    	MESSAGE(ERROR "Problem with find python")
			endif()
			math(EXPR __next_index__ "${__index__}+1")
			list(GET PYTHON_LIBRARY ${__next_index__} PYTHON_DEBUG_LIBRARY)
		endif()
		  	
		if (SWIG_PYTHON_INTERPRETER_NO_DEBUG)
		  	set(PYTHON_DEBUG_LIBRARY ${PYTHON_RELEASE_LIBRARY})
		endif()

	  	set_target_properties(Imported::Python PROPERTIES
	  	IMPORTED_IMPLIB_DEBUG           ${PYTHON_DEBUG_LIBRARY}
	  	IMPORTED_IMPLIB_RELEASE         ${PYTHON_RELEASE_LIBRARY}
	  	IMPORTED_IMPLIB_RELWITHDEBINFO  ${PYTHON_RELEASE_LIBRARY})
	else()
		set_target_properties(Imported::Python PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 
	endif()
		
	EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE}  -c  "import site;print(site.getsitepackages()[-1])"  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "PYTHON_SITE_PACKAGES_DIR ${PYTHON_SITE_PACKAGES_DIR}")
	
endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToLibrary Name)

	if (WIN32)
		target_link_libraries(${Name} PUBLIC Imported::Python)
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
	
endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToExecutable Name)

	if (WIN32)
		# already linked
		elseif (APPLE)
		target_link_libraries(${Name} PUBLIC Imported::Python)
	else()
		# for nix is trickier since the linking order is important
		# the "-Wl,--start-group" does not always work (for example in travis)
		# see http://cmake.3232098.n2.nabble.com/Link-order-Ubuntu-tt7598592.html
		# i found this trick: VisusKernel should appear always before python 
		target_link_libraries(${Name} PUBLIC $<TARGET_FILE:VisusKernel> Imported::Python)
	endif()
		
endmacro()

# ///////////////////////////////////////////////////
macro(AddStaticLibrary Name)

	add_library(${Name} STATIC ${ARGN})

	# this fixes the problem of static symbols conflicting with dynamic lib
	# example: dynamic libcrypto conflicting with internal static libcrypto
	if (NOT WIN32)
		target_compile_options(${Name} PRIVATE -fvisibility=hidden)
	endif()

	SetupCommonOutputTargetProperties(${Name})
endmacro()

# ///////////////////////////////////////////////////
macro(AddLibrary Name)
	add_library(${Name} ${ARGN})
	SetupCommonCompileOptions(${Name})
	set_target_properties(${Name} PROPERTIES FOLDER "")
	string(TOUPPER ${Name} __upper_case__name__)
	target_compile_definitions(${Name}  PRIVATE VISUS_BUILDING_${__upper_case__name__}=1)
	target_include_directories(${Name}  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
	LinkPythonToLibrary(${Name})
	SetupCommonOutputTargetProperties(${Name})
endmacro()


# ///////////////////////////////////////////////////
macro(AddExecutable Name)
	add_executable(${Name} ${ARGN})
	SetupCommonCompileOptions(${Name})
	set_target_properties(${Name} PROPERTIES FOLDER "Executable/")
	LinkPythonToExecutable(${Name})
	SetupCommonOutputTargetProperties(${Name})
endmacro()

# ///////////////////////////////////////////////////
macro(AddSwigLibrary NamePy WrappedLib SwigFile)

	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})

	# this is for *.py generated files
	string(REPLACE "$<CONFIG>" "${CMAKE_CFG_INTDIR}" CMAKE_SWIG_OUTDIR ${PYTHON_BINARY_DIR})

	# this is for generated C++ and header files
	if (CMAKE_CONFIGURATION_TYPES)
		set(SWIG_OUTFILE_DIR  ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})
	else()
		set(SWIG_OUTFILE_DIR  ${CMAKE_BINARY_DIR})
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
		
	# important to share types between modules
	set_source_files_properties (${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS "-DSWIG_TYPE_TABLE=${SWIG_TYPE_TABLE}")	
		
	if (TARGET _${NamePy})
		set(RealName _${NamePy})
	else()
		set(RealName ${NamePy})
	endif()

	set_target_properties(${RealName} PROPERTIES FOLDER Swig/)

	# disable warnings
	if (WIN32)
		target_compile_definitions(${RealName}  PRIVATE /W0)
		if (SWIG_PYTHON_INTERPRETER_NO_DEBUG)
			target_compile_definitions(${RealName} PUBLIC -DSWIG_PYTHON_INTERPRETER_NO_DEBUG=1) 
		endif()
	else()
		set_target_properties(${RealName} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()

	LinkPythonToLibrary(${RealName})
	SetupCommonCompileOptions(${RealName})
	SetupCommonOutputTargetProperties(${RealName})

	target_link_libraries(${RealName} PUBLIC ${WrappedLib})
	
endmacro()


# //////////////////////////////////////////////////////////////////////////
macro(FindGit)
	find_program(GIT_CMD git REQUIRED)
	find_package_handle_standard_args(GIT REQUIRED_VARS GIT_CMD)
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(FindGitRevision)
	FindGit()
	execute_process(COMMAND ${GIT_CMD} rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "Current GIT_REVISION ${GIT_REVISION}")
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(FindVCPKGDir)
	set(VCPKG_DIR ${CMAKE_TOOLCHAIN_FILE}/../../../installed/${VCPKG_TARGET_TRIPLET})
	get_filename_component(VCPKG_DIR ${VCPKG_DIR} REALPATH)	
endmacro()

