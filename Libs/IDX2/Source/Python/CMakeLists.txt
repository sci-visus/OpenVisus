project(idx2Py)
cmake_minimum_required(VERSION 3.17)

add_subdirectory(nanobind)

find_package(Python REQUIRED COMPONENTS Development)

nanobind_add_module(
  idx2Py
  NB_STATIC # Build static libnanobind (the extension module itself remains a shared library)
  idx2Py.cpp
)

include_directories(${idx2Lib_SOURCE_DIR})
set_target_properties(idx2Py PROPERTIES CMAKE_POSITION_INDEPENDENT_CODE ON)
target_link_libraries(idx2Py PUBLIC idx2)


install(TARGETS idx2Py LIBRARY DESTINATION .)
