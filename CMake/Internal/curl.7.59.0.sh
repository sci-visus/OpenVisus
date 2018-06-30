#!/bin/bash

set -e

INSTALL_DIR=$1
OPENSSL_DIR=$2

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp 
curl https://curl.haxx.se/download/curl-7.59.0.tar.gz -O 
tar xvf curl-7.59.0.tar.gz  && cd curl-7.59.0
./configure --prefix=$INSTALL_DIR --with-ssl=$OPENSSL_DIR
make  -j 4
make install

cd /tmp 
rm -Rf curl-7.59.0
