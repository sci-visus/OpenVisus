#!/bin/bash

INSTALL_DIR=$1

set -e

curl https://curl.haxx.se/download/curl-7.59.0.tar.gz -O
tar xvf curl-7.59.0.tar.gz  && cd curl-7.59.0 

cmake . \
  -DCMAKE_POSITION_INDEPENDENT_CODE=1 \
  -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
  -DOPENSSL_INCLUDE_DIR=$INSTALL_DIR/include \
  -DOPENSSL_SSL_LIBRARY=$INSTALL_DIR/lib/libssl.so \
  -DOPENSSL_CRYPTO_LIBRARY=$INSTALL_DIR/lib/libcrypto.so 
  
cmake --build . --target all -- -j 4  
cmake --build . --target install 

cd ../ && rm -Rf curl-7.59.0

