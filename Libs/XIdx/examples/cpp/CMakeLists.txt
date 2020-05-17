#include_directories(${LIBXML2_INCLUDE_DIR})

#add_executable(validate validate.cpp)

#target_link_libraries(validate ${LIBXML2_LIBRARIES})

add_executable(write write.cpp)
target_link_libraries(write PUBLIC VisusXIdx Python::Python)

add_executable(read read.cpp)
target_link_libraries(read PUBLIC VisusXIdx Python::Python)
