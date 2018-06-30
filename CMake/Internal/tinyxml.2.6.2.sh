#!/bin/bash

set -e

INSTALL_DIR=$1

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp 
curl "https://netcologne.dl.sourceforge.net/project/tinyxml/tinyxml/2.6.2/tinyxml_2_6_2.tar.gz" -O
tar xvzf tinyxml_2_6_2.tar.gz && cd tinyxml

cat > CMakeLists.txt << EOF 
CMAKE_MINIMUM_REQUIRED(VERSION 2.6) 
project(tinyxml)
add_library(tinyxml SHARED tinyxml.cpp tinyxmlparser.cpp xmltest.cpp tinyxmlerror.cpp  tinystr.cpp)
install(TARGETS tinyxml DESTINATION lib)
file(GLOB HEADERS *.h)
install(FILES \${HEADERS} DESTINATION include)
EOF

cmake . -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
make -j 4
make install

cd /tmp 
rm -Rf tinyxml
