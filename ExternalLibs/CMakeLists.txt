# if XIDX is not cloned automatically clone it
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/xidx/include")
    message("Unable to find XIDX, updating submodule..")

    execute_process(COMMAND git submodule update --init -- xidx
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

add_subdirectory(xidx)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/xidx/include/xidx
        ${LIBXML2_INCLUDE_DIR})

AddSwigLibrary(xidx xidx ${CMAKE_CURRENT_SOURCE_DIR}/xidx/include/xidx/xidx.i)