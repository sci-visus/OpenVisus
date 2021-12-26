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


DownloadFile "https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz" 
tar xzf patchelf-*.tar.gz
pushd patchelf-*
./configure 
make 
make install 
autoreconf -f -i  
./configure 
make
make install
popd
rm -Rf patchelf-*


