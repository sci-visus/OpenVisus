#!/bin/bash

INSTALL_DIR=$1

set -e
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
cmake --build . --target all -- -j 4 
cmake --build . --target install 

cd ../ && rm -Rf tinyxml
	

