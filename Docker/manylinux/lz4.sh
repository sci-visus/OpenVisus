#!/bin/bash

INSTALL_DIR=$1

set -e
curl https://sources.voidlinux.eu/lz4-1.8.1.2/v1.8.1.2.tar.gz -O 
tar xvzf v1.8.1.2.tar.gz   && cd lz4-1.8.1.2 

cat > CMakeLists.txt << EOF
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 
project(lz4)
add_definitions(-DXXH_NAMESPACE=LZ4_)
file(GLOB Headers lib/*.h)
file(GLOB Sources lib/*.c)
include_directories(./lib)
add_library(lz4 SHARED \${Headers} \${Sources})
install(TARGETS lz4 DESTINATION lib)
install(FILES \${Headers} DESTINATION include)
EOF

cmake . -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR 
cmake --build . --target all -- -j 4  
cmake --build . --target install 

cd ../ && rm -Rf lz4-1.8.1.2
	
