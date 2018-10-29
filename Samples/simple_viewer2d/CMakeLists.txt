CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

project(simple_viewer2d)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenVisus REQUIRED)

file(GLOB Sources *.h *.cpp)

if (APPLE)
  set(EXECUTABLE_TYPE "MACOSX_BUNDLE")
endif()

add_executable(external_simple_viewer2d ${EXECUTABLE_TYPE} ${Sources})

target_link_libraries(external_simple_viewer2d PRIVATE OpenVisus::Kernel OpenVisus::Idx OpenVisus::Db OpenVisus::Gui OpenVisus::GuiNodes OpenVisus::AppKit)

