include_directories(${LIBXML2_INCLUDE_DIR})

add_executable(validate validate.cpp)

target_link_libraries(validate ${LIBXML2_LIBRARIES})

add_executable(write write.cpp)
target_link_libraries(write ${LIBXML2_LIBRARIES} xidx)

add_executable(read read.cpp)
target_link_libraries(read ${LIBXML2_LIBRARIES} xidx)
