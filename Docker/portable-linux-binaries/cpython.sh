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

VERSION=$1

DownloadFile https://www.python.org/ftp/python/${VERSION}/Python-${VERSION}.tgz 
tar xzf Python-${VERSION}.tgz
pushd Python-${VERSION}
./configure --enable-shared
make
make altinstall
popd

/usr/local/bin/python${VERSION:0:1}.${VERSION:2:1} -m pip install -q --upgrade pip
/usr/local/bin/python${VERSION:0:1}.${VERSION:2:1} -m pip install -q numpy setuptools wheel twine 

# clean up
rm -Rf ./Python-${VERSION}*


