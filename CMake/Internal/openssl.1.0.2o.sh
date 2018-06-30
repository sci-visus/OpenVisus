#!/bin/bash

set -e

INSTALL_DIR=$1

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp 
curl https://www.openssl.org/source/openssl-1.0.2o.tar.gz -O 
tar xvf openssl-1.0.2o.tar.gz && cd openssl-1.0.2o
./config --prefix=$INSTALL_DIR shared                                      
make -j 4  
make install   

cd /tmp 
rm -Rf openssl-1.0.2o



