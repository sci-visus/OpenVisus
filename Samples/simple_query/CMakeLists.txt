cmake_minimum_required(VERSION 3.1)

project(simple_query)

set(CMAKE_CXX_STANDARD 11)

find_package(Visus REQUIRED)

file(GLOB Sources *.h *.cpp)

add_executable(external_simple_query ${Sources})

target_link_libraries(external_simple_query VisusKernel VisusIdx VisusDb)

