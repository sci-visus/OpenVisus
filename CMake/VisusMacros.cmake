

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

		# save libraries and binaries in the same directory        
		set(EXECUTABLE_OUTPUT_PATH              ${CMAKE_BINARY_DIR})           
		set(LIBRARY_OUTPUT_PATH                 ${CMAKE_BINARY_DIR})	
		set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR})
		
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
		target_compile_options(${Name} PRIVATE  -Wno-unused-variable -Wno-reorder)	
	
	else()
	
		# enable 64 bit file support (see http://learn-from-the-guru.blogspot.it/2008/02/large-file-support-in-linux-for-cc.html)
		target_compile_options(${Name} PRIVATE -D_FILE_OFFSET_BITS=64)
		# -Wno-attributes to suppress spurious "type attributes ignored after type is already defined" messages 
		# see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39159
		target_compile_options(${Name} PRIVATE -Wno-attributes)	
	
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
	find_package(NumPy                          REQUIRED)

	message(STATUS "PYTHON_EXECUTABLE   ${PYTHON_EXECUTABLE}")
	message(STATUS "PYTHON_LIBRARY      ${PYTHON_LIBRARY}")
	message(STATUS "PYTHON_INCLUDE_DIR  ${PYTHON_INCLUDE_DIR}")
	message(STATUS "NUMPY_FOUND         ${NUMPY_FOUND}")
	message(STATUS "NUMPY_VERSION       ${NUMPY_VERSION}")
	message(STATUS "NUMPY_INCLUDE_DIR   ${NUMPY_INCLUDE_DIR}")	
	
	add_library(OpenVisus::Python SHARED IMPORTED GLOBAL)
	set_property(TARGET OpenVisus::Python APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${PYTHON_INCLUDE_DIRS}")	
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
	  	set_target_properties(OpenVisus::Python PROPERTIES
				IMPORTED_IMPLIB_DEBUG           ${PYTHON_DEBUG_LIBRARY}
				IMPORTED_IMPLIB_RELEASE         ${PYTHON_RELEASE_LIBRARY}
				IMPORTED_IMPLIB_RELWITHDEBINFO  ${PYTHON_RELEASE_LIBRARY})
	else()
		set_target_properties(OpenVisus::Python PROPERTIES IMPORTED_LOCATION ${PYTHON_LIBRARY}) 
	endif()
	

	EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE}  -c  "import site;print(site.getsitepackages()[-1])"  OUTPUT_VARIABLE PYTHON_SITE_PACKAGES_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "PYTHON_SITE_PACKAGES_DIR ${PYTHON_SITE_PACKAGES_DIR}")

endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToLibrary Name)
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
	
endmacro()

# ///////////////////////////////////////////////////
macro(LinkPythonToExecutable Name)
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
endmacro()


# ///////////////////////////////////////////////////
macro(AddLibrary Name)
	add_library(${Name} ${ARGN})
	SetupCommonCompileOptions(${Name})
	set_target_properties(${Name} PROPERTIES FOLDER "${CMAKE_FOLDER_PREFIX}")
	string(TOUPPER ${Name} __upper_case__name__)
	target_compile_definitions(${Name}  PRIVATE VISUS_BUILDING_${__upper_case__name__}=1)
	target_include_directories(${Name}  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:include>)
	LinkPythonToLibrary(${Name})
  if (WIN32)
	 	set_target_properties(${Name}
	      PROPERTIES
	      COMPILE_PDB_NAME_DEBUG          ${Name}
	      COMPILE_PDB_NAME_RELEASE        ${Name}
	      COMPILE_PDB_NAME_MINSIZEREL     ${Name}
	      COMPILE_PDB_NAME_RELWITHDEBINFO ${Name})
	endif()
	InstallLibrary(${Name})
endmacro()


# ///////////////////////////////////////////////////
macro(AddExecutable Name)
	add_executable(${Name} ${ARGN})
	SetupCommonCompileOptions(${Name})
	set_target_properties(${Name} PROPERTIES FOLDER "${CMAKE_FOLDER_PREFIX}Executable/")
	LinkPythonToExecutable(${Name})
	if (WIN32)
	 	set_target_properties(${Name}
	      PROPERTIES
	      COMPILE_PDB_NAME_DEBUG          ${Name}
	      COMPILE_PDB_NAME_RELEASE        ${Name}
	      COMPILE_PDB_NAME_MINSIZEREL     ${Name}
	      COMPILE_PDB_NAME_RELWITHDEBINFO ${Name})
	endif()
	InstallExecutable(${Name})
endmacro()

# ///////////////////////////////////////////////////
macro(AddSwigLibrary NamePy WrappedLib SwigFile)

	find_package(SWIG 3.0 REQUIRED)
	include(${SWIG_USE_FILE})
	set(CMAKE_SWIG_OUTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})
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
	set_source_files_properties (${swig_generated_file_fullname} PROPERTIES COMPILE_FLAGS "-DSWIG_TYPE_TABLE=OpenVisus")	
	
	if (TARGET _${NamePy})
	  set(RealName _${NamePy})
	else()
	  set(RealName ${NamePy})
	endif()

	SetupCommonCompileOptions(${RealName})
	set_target_properties(${RealName} PROPERTIES FOLDER ${CMAKE_FOLDER_PREFIX}Swig/)
	target_include_directories(${RealName} PUBLIC ${NUMPY_INCLUDE_DIR})

	# disable warnings
	if (WIN32)
		target_compile_definitions(${RealName}  PRIVATE /W0)
	else()
		set_target_properties(${RealName} PROPERTIES COMPILE_FLAGS "${BUILD_FLAGS} -w")
	endif()

	LinkPythonToLibrary(${RealName})
	target_link_libraries(${RealName} PUBLIC ${WrappedLib})

	if (WIN32)
		set_target_properties(${RealName}
	      PROPERTIES
	      COMPILE_PDB_NAME_DEBUG          ${RealName}_d
	      COMPILE_PDB_NAME_RELEASE        ${RealName}
	      COMPILE_PDB_NAME_MINSIZEREL     ${RealName}
	      COMPILE_PDB_NAME_RELWITHDEBINFO ${RealName})
		set_target_properties(${RealName} PROPERTIES DEBUG_POSTFIX  "_d")
	endif()	

	InstallLibrary(${RealName})
	
endmacro()

# ///////////////////////////////////////////////////
macro(InstallLibrary Name)
 
	if (WIN32)
	
		install(TARGETS ${Name}  CONFIGURATIONS DEBUG          RUNTIME DESTINATION debug/bin   LIBRARY DESTINATION debug/bin ARCHIVE DESTINATION debug/lib)
		install(TARGETS ${Name}  CONFIGURATIONS RELEASE        RUNTIME DESTINATION       bin   LIBRARY DESTINATION       bin ARCHIVE DESTINATION       lib)	
		install(TARGETS ${Name}  CONFIGURATIONS RELWITHDEBINFO RUNTIME DESTINATION       bin   LIBRARY DESTINATION       bin ARCHIVE DESTINATION       lib)
						
		install(FILES $<TARGET_PDB_FILE:${Name}> CONFIGURATIONS DEBUG          DESTINATION debug/bin)
		install(FILES $<TARGET_PDB_FILE:${Name}> CONFIGURATIONS RELEASE        DESTINATION       bin)
		install(FILES $<TARGET_PDB_FILE:${Name}> CONFIGURATIONS RELWITHDEBINFO DESTINATION       bin)	
	
	elseif (APPLE)

		install(TARGETS ${Name}  CONFIGURATIONS DEBUG          LIBRARY DESTINATION debug/bin RUNTIME DESTINATION debug/bin BUNDLE DESTINATION debug/bin ARCHIVE DESTINATION debug/lib)
		install(TARGETS ${Name}  CONFIGURATIONS RELEASE        LIBRARY DESTINATION bin       RUNTIME DESTINATION bin       BUNDLE DESTINATION bin       ARCHIVE DESTINATION lib)
		install(TARGETS ${Name}  CONFIGURATIONS RELWITHDEBINFO LIBRARY DESTINATION bin       RUNTIME DESTINATION bin       BUNDLE DESTINATION bin       ARCHIVE DESTINATION lib)
		
	else()
	
		install(TARGETS ${Name} LIBRARY DESTINATION bin RUNTIME DESTINATION bin ARCHIVE DESTINATION lib)
		
	endif()

endmacro()


# ///////////////////////////////////////////////////
macro(InstallExecutable Name)

	if (WIN32)

		install(TARGETS ${Name} CONFIGURATIONS DEBUG          RUNTIME    DESTINATION  debug/bin)		
		install(TARGETS ${Name} CONFIGURATIONS RELEASE        RUNTIME    DESTINATION        bin)			
		install(TARGETS ${Name} CONFIGURATIONS RELWITHDEBINFO RUNTIME    DESTINATION        bin)						

		install(FILES $<TARGET_PDB_FILE:${Name}> CONFIGURATIONS  DEBUG          DESTINATION debug/bin)
		install(FILES $<TARGET_PDB_FILE:${Name}> CONFIGURATIONS  RELEASE        DESTINATION       bin)
		install(FILES $<TARGET_PDB_FILE:${Name}> CONFIGURATIONS  RELWITHDEBINFO DESTINATION       bin)

	elseif (APPLE)
	
		install(TARGETS ${Name} CONFIGURATIONS DEBUG          BUNDLE     DESTINATION  debug/bin RUNTIME    DESTINATION  debug/bin)
		install(TARGETS ${Name} CONFIGURATIONS RELEASE        BUNDLE     DESTINATION  bin       RUNTIME    DESTINATION  bin)
		install(TARGETS ${Name} CONFIGURATIONS RELWITHDEBINFO BUNDLE     DESTINATION  bin       RUNTIME    DESTINATION  bin)
	
	else()
		
		install(TARGETS ${Name} RUNTIME DESTINATION  bin)			
	endif()

endmacro()

# ///////////////////////////////////////////////////
macro(InstallBuildFiles Pattern Destination)

	if (CMAKE_CONFIGURATION_TYPES)
		install(CODE "
			FILE(GLOB __files__ ${CMAKE_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${Pattern})
			MESSAGE(STATUS \"InstallBuildFiles from ${CMAKE_BINARY_DIR}/\${CMAKE_INSTALL_CONFIG_NAME}/${Pattern}\")
			MESSAGE(STATUS \"Files: \${__files__} \")
			FILE(INSTALL \${__files__} DESTINATION \"${CMAKE_INSTALL_PREFIX}/${Destination}\")
		")
	else()
		install(CODE "
			FILE(GLOB __files__ ${CMAKE_BINARY_DIR}/${Pattern})
			MESSAGE(STATUS \"InstallBuildFiles from ${CMAKE_BINARY_DIR}/${Pattern}\")
			MESSAGE(STATUS \"Files: \${__files__} \")
			FILE(INSTALL \${__files__} DESTINATION \"${CMAKE_INSTALL_PREFIX}/${Destination}\")
		")
	endif()
endmacro()


# //////////////////////////////////////////////////////////////////////////
macro(AddExternalApp name SourceDir BinaryDir)
	if (WIN32 OR APPLE)
		set(CMAKE_GENERATOR_ARGUMENT -G"${CMAKE_GENERATOR}")
	else()
		set(CMAKE_GENERATOR_ARGUMENT -G"\"${CMAKE_GENERATOR}\"")
	endif()
	add_custom_target(${name} 
	          "${CMAKE_COMMAND}" -E echo "Running ${name} ..."
		COMMAND "${CMAKE_COMMAND}"  "${CMAKE_GENERATOR_ARGUMENT}" -H"${SourceDir}/"  -B"${BinaryDir}/"  -DQt5_DIR="${Qt5_DIR}" -DOpenVisus_DIR=${CMAKE_INSTALL_PREFIX}
		COMMAND "${CMAKE_COMMAND}"  --build "${BinaryDir}/" --config ${CMAKE_BUILD_TYPE})
	set_target_properties(${name} PROPERTIES FOLDER CMakeTargets/)
endmacro()


# //////////////////////////////////////////////////////////////////////////
macro(AddCTest Name Command WorkingDirectory)

	add_test(NAME ${Name} WORKING_DIRECTORY "${WorkingDirectory}" COMMAND "${Command}" ${ARGN})

	set(options "CTEST_OUTPUT_ON_FAILURE=1")
	
	if (CMAKE_CONFIGURATION_TYPES)
		list(APPEND __options__ "PYTHONPATH=${CMAKE_BINARY_DIR}/$<CONFIG>")
	else()
		list(APPEND __options__ "PYTHONPATH=${CMAKE_BINARY_DIR}")
	endif()

	if (WIN32)
	
		if (VISUS_GUI)
			list(APPEND __options__ "PATH=${Qt5_DIR}\\..\\..\\..\\bin")
		endif()
		
	elseif(APPLE)

	else()
		list(APPEND __OPTIONS__ "LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}")
	endif()
	
	set_tests_properties(${Name} PROPERTIES ENVIRONMENT "${__options__}")

endmacro()



# //////////////////////////////////////////////////////////////////////////
macro(Win32AddImportedLibrary name include_dir debug_lib release_lib debug_dlls release_dlls)
	add_library(${name} SHARED IMPORTED GLOBAL)
	set_target_properties(${name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${include_dir}")
	set_property(TARGET ${name} APPEND PROPERTY IMPORTED_CONFIGURATIONS "Debug;Release;RelWithDebInfo")
	set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB_DEBUG           ${debug_lib})
	set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB_RELEASE         ${release_lib})    
	set_target_properties(${name} PROPERTIES IMPORTED_IMPLIB_RELWITHDEBINFO  ${release_lib}) 
	
	foreach(it ${debug_dlls})
		file(COPY ${it} DESTINATION ${LIBRARY_OUTPUT_PATH}/Debug)
	endforeach()
	
	foreach(it ${release_dlls})
		file(COPY ${it} DESTINATION ${LIBRARY_OUTPUT_PATH}/RelWithDebInfo)
		file(COPY ${it} DESTINATION ${LIBRARY_OUTPUT_PATH}/Release)
	endforeach()	
	
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(Win32CopyDllToBuild target debug_dll release_dll)

	ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD COMMAND ${CMAKE_COMMAND}  -E 
		$<$<CONFIG:Debug>:copy>  $<$<CONFIG:Debug>:${debug_dll}> $<$<CONFIG:Debug>:${LIBRARY_OUTPUT_PATH}/Debug>
		$<$<CONFIG:Release>:echo>  $<$<CONFIG:Release>:"no command">
		$<$<CONFIG:RelWithDebInfo>:echo> $<$<CONFIG:RelWithDebInfo>:"no command">
	)
		
	ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD COMMAND  ${CMAKE_COMMAND}  -E 
		$<$<CONFIG:Debug>:echo>  $<$<CONFIG:Debug>:"no command">
		$<$<CONFIG:Release>:copy> $<$<CONFIG:Release>:${release_dll}> $<$<CONFIG:Release>:${LIBRARY_OUTPUT_PATH}/Release>
		$<$<CONFIG:RelWithDebInfo>:echo> $<$<CONFIG:RelWithDebInfo>:"no command">
	)
		
	ADD_CUSTOM_COMMAND(TARGET ${target} POST_BUILD COMMAND  ${CMAKE_COMMAND}  -E 
		$<$<CONFIG:Debug>:echo>  $<$<CONFIG:Debug>:"no command">
		$<$<CONFIG:Release>:echo> $<$<CONFIG:Release>:"no command">
		$<$<CONFIG:RelWithDebInfo>:copy> $<$<CONFIG:RelWithDebInfo>:${release_dll}> $<$<CONFIG:RelWithDebInfo>:${LIBRARY_OUTPUT_PATH}/RelWithDebInfo>
	)		
		
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(FindGitRevision)
	find_program(GIT_CMD git REQUIRED)
	find_package_handle_standard_args(GIT REQUIRED_VARS GIT_CMD)
	execute_process(COMMAND ${GIT_CMD} rev-parse --short HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
	message(STATUS "Current GIT_REVISION ${GIT_REVISION}")
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(FindVCPKGDir)
	set(VCPKG_DIR ${CMAKE_TOOLCHAIN_FILE}/../../../installed/${VCPKG_TARGET_TRIPLET})
	get_filename_component(VCPKG_DIR ${VCPKG_DIR} REALPATH)	
endmacro()

# //////////////////////////////////////////////////////////////////////////
macro(InstallPostInstallStep)
	install(CODE "
		execute_process( 
			 COMMAND \"${PYTHON_EXECUTABLE}\" -u \"${CMAKE_INSTALL_PREFIX}/configure.py\"
			 	--qt5-dir=\"${Qt5_DIR}\"
				cmake_post_install)
	")
endmacro()

