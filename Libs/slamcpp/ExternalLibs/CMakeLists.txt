
if (CMAKE_CONFIGURATION_TYPES)
	SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/ExternalLib )
else()
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/ExternalLib ) 
endif()

if(WIN32)
	add_compile_options("/W0")
else()
	add_compile_options("-w")
	add_compile_options(-fvisibility=hidden) # this fixes the problem of static symbols conflicting with dynamic lib
endif()

# ///////////////////////////////////////////////////
macro(AddExternalLibrary Name)
	add_library(${Name} STATIC ${ARGN})
	set_target_properties(${Name} PROPERTIES FOLDER "ExternalLibs/g2o/")
endmacro()

add_subdirectory(g2o)