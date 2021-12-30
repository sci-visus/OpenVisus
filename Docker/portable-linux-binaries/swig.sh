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

DownloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"
tar xzf swig-*.tar.gz  
pushd swig-*
DownloadFile  "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" 
./Tools/pcre-build.sh 
./configure --prefix=/usr
make 
make install
popd

rm -Rf swig-*

