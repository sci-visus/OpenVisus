#!/bin/bash

set -e

INSTALL_DIR=$1

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp 
curl https://sources.voidlinux.eu/lz4-1.8.1.2/v1.8.1.2.tar.gz -O
tar xvzf v1.8.1.2.tar.gz   && cd lz4-1.8.1.2

cat > CMakeLists.txt << EOF 
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 
project(lz4)
ADD_DEFINITIONS(-DXXH_NAMESPACE=LZ4_)
include_directories(./lib)
file(GLOB SOURCES lib/*.c)
file(GLOB HEADERS lib/*.h)
add_library(lz4 SHARED \${SOURCES})
install(TARGETS lz4 DESTINATION lib)
install(FILES \${HEADERS} DESTINATION include)
EOF

cmake . -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
make -j 4
make install

cd /tmp 
rm -Rf lz4-1.8.1.2

