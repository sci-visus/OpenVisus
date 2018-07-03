#!/bin/bash

INSTALL_DIR=$1

set -e
curl https://zlib.net/zlib-1.2.11.tar.gz -O
tar xvf zlib-1.2.11.tar.gz  && cd zlib-1.2.11 
cmake . -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR 
cmake --build . --target all -- -j 4 
cmake --build . --target install 

cd ../ && rm -Rf zlib-1.2.11
	
