cmake_minimum_required(VERSION 3.1)

project(simple_query)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenVisus REQUIRED)

file(GLOB Sources *.h *.cpp)
add_executable(simple_query ${Sources})
target_link_libraries(simple_query PUBLIC OpenVisus::Idx)
