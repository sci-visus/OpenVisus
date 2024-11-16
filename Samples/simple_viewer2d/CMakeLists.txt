CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

project(simple_viewer2d)
set(CMAKE_CXX_STANDARD 11)

find_package(OpenVisus REQUIRED)

add_executable(simple_viewer2d main.cpp)
target_link_libraries(simple_viewer2d PUBLIC OpenVisus::Gui)

  
