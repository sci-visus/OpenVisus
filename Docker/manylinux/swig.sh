#!/bin/bash

INSTALL_DIR=$1

set -e
curl "https://vorboss.dl.sourceforge.net/project/swig/swig/swig-3.0.12/swig-3.0.12.tar.gz" -O 
tar xvzf swig-3.0.12.tar.gz && cd swig-3.0.12 

curl "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" -O
./Tools/pcre-build.sh

./configure --prefix=$INSTALL_DIR 
make -j 4 
make install 

cd ../ && rm -Rf swig-3.0.12

