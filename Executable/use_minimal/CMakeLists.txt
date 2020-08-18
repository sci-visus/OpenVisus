
# show the use of minimal wrapper for old compiler'
FILE(GLOB Sources *.h *.cpp)
add_executable(use_minimal ${Sources})
target_link_libraries(use_minimal PUBLIC VisusMinimal)

set_property(TARGET use_minimal PROPERTY CXX_STANDARD 98)
set_property(TARGET use_minimal PROPERTY CXX_STANDARD_REQUIRED ON)





