CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

if (POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW) # windows CMAKE_MSVC_RUNTIME_LIBRARY
endif()

# this must be BEFORE PROJECT command
SET(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

PROJECT(TEST_STATIC_LINKING)

set(BUILD_SHARED_LIBS OFF) 
set(CMAKE_CXX_STANDARD              11)
set(CMAKE_CXX_STANDARD_REQUIRED     ON)

file(GLOB SOURCES *.cpp *.h)
add_executable (test_static_linking ${SOURCES})

FIND_PATH(OPENVISUS_DIR include/Kernel/Visus/Kernel.h REQUIRED)

target_include_directories(test_static_linking PUBLIC 
	${OPENVISUS_DIR}/include/Kernel 
	${OPENVISUS_DIR}/include/Db)

target_link_libraries(test_static_linking  PUBLIC 
	${OPENVISUS_DIR}/lib/VisusKernel.lib 
	${OPENVISUS_DIR}/lib/VisusDb.lib)

if (MSVC)
	target_link_libraries(test_static_linking PUBLIC Psapi.lib Iphlpapi.lib DbgHelp.lib Ws2_32.lib advapi32.lib)
endif()