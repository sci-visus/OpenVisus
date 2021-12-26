#!/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

function DownloadFile {
	curl -L --insecure --retry 3 $1 -O		
}

DownloadFile https://www.openssl.org/source/openssl-1.0.2n.tar.gz
tar xzf openssl-*.tar.gz  
pushd openssl-* 
./config -fpic shared --prefix=/usr --openssldir=/usr 
make 
make install 
popd

rm -Rf openssl-* 


