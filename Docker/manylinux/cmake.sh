#!/bin/bash

INSTALL_DIR=$1

set -e
curl https://cmake.org/files/v3.12/cmake-3.12.0-rc1.tar.gz -O
tar xvzf cmake-3.12.0-rc1.tar.gz && cd cmake-3.12.0-rc1 
./bootstrap --prefix=$INSTALL_DIR 
make -j 8  
make install  
hash -r  

cd .. && rm -Rf cmake-3.12.0-rc1

