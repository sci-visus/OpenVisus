cmake_minimum_required(VERSION 3.8)
project(idx2Applications CXX)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(idx2App idx2App.cpp)
target_link_libraries(idx2App idx2 Threads::Threads)

add_executable(idx2App_v2 idx2App_v2.cpp)
target_link_libraries(idx2App_v2 idx2 Threads::Threads)

add_executable(idx2Samples idx2Samples.cpp)
target_link_libraries(idx2Samples idx2 Threads::Threads)

add_executable(idx2CompareVolumes idx2CompareVolumes.cpp)
target_link_libraries(idx2App idx2 Threads::Threads)
