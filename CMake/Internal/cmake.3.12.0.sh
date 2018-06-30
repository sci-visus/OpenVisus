#!/bin/bash

set -e

INSTALL_DIR=$1
OPENSSL_DIR=$2

export LD_LIBRARY_PATH=$OPENSSL_DIR/lib

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp 
curl https://cmake.org/files/v3.12/cmake-3.12.0-rc1.tar.gz -O 
tar xvzf cmake-3.12.0-rc1.tar.gz && cd cmake-3.12.0-rc1
./bootstrap
make
./bin/cmake -DCMAKE_USE_OPENSSL=1 -DOPENSSL_ROOT_DIR=$OPENSSL_DIR -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
make  -j 4
make install

cd /tmp 
rm -Rf cmake-3.12.0-rc1