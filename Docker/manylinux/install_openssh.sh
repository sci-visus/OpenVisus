#!/bin/bash

set -e
set -x

curl -L --insecure --retry 3 https://www.openssl.org/source/openssl-1.0.2n.tar.gz | tar xzf -
pushd openssl-* 
./config -fpic shared --prefix=/usr --openssldir=/usr 
make 
make install 
popd 
rm -Rf openssl-*