#!/bin/bash

INSTALL_DIR=$1

set -e
curl https://www.openssl.org/source/openssl-1.0.2o.tar.gz -O 
tar xvf openssl-1.0.2o.tar.gz && cd openssl-1.0.2o 
./config --prefix=$INSTALL_DIR shared                           
make -j 4   
make install 

cd ../ && rm -Rf openssl-1.0.2o
