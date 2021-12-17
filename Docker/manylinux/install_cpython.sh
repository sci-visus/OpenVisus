#!/bin/bash

set -e
set -x

VERSION=$1
curl -L --insecure --retry 3 "https://www.python.org/ftp/python/${VERSION}/Python-${VERSION}.tgz" | tar xzf -
pushd Python-${VERSION}
./configure --with-ensurepip=yes --enable-shared LDFLAGS="-L/usr/local/lib -L/usr/local/lib64 -Wl,-rpath,/usr/local/lib -Wl,-rpath,/usr/local/lib64" 
make 
make altinstall 
popd 
rm -Rf ./Python-${VERSION}* 
/usr/local/bin/python${VERSION:0:1}.${VERSION:2:1} -m pip install -q --upgrade pip 
/usr/local/bin/python${VERSION:0:1}.${VERSION:2:1} -m pip install -q numpy setuptools wheel twine