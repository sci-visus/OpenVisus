#!/bin/bash

set -e

INSTALL_DIR=$1

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp 
curl https://zlib.net/zlib-1.2.11.tar.gz -O
tar xvf zlib-1.2.11.tar.gz  && cd zlib-1.2.11
cmake . -DCMAKE_POSITION_INDEPENDENT_CODE=1 -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
make -j 4
make install

cd /tmp 
rm -Rf zlib-1.2.11

