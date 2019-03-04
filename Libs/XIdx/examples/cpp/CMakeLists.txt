#include_directories(${LIBXML2_INCLUDE_DIR})

#add_executable(validate validate.cpp)

#target_link_libraries(validate ${LIBXML2_LIBRARIES})

AddExecutable(write write.cpp)
target_link_libraries(write PUBLIC VisusXIdx)

AddExecutable(read read.cpp)
target_link_libraries(read PUBLIC VisusXIdx)
