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


DownloadFile "https://www.zlib.net/zlib-1.2.11.tar.gz" 
tar xzf zlib-*.tar.gz
pushd zlib-*
./configure 
make 
make install 
popd

rm -Rf zlib-*


