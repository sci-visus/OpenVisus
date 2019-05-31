
find_path(OpenVisus_DIR Names OpenVisusConfig.cmake NO_DEFAULT_PATH)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVisus DEFAULT_MSG OpenVisus_DIR)
if(OpenVisus_FOUND)
	if (WIN32)
		string(REPLACE "\\" "/" OpenVisus_DIR "${OpenVisus_DIR}")
	endif()
	message(STATUS "OpenVisus found in ${OpenVisus_DIR}")
	include(${OpenVisus_DIR}/CMake/VisusMacros.cmake)
	AddOpenVisusPythonLibraries(${OpenVisus_DIR})
endif()




