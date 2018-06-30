#!/bin/bash

set -e

INSTALL_DIR=$1

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp
curl https://vorboss.dl.sourceforge.net/project/swig/swig/swig-3.0.12/swig-3.0.12.tar.gz -O 
tar xvf swig-3.0.12.tar.gz  && cd swig-3.0.12
curl https://netcologne.dl.sourceforge.net/project/pcre/pcre/8.42/pcre-8.42.tar.gz -O
./Tools/pcre-build.sh                           
./configure --prefix=$INSTALL_DIR
make    -j 4                                         
make install 

cd /tmp 
rm -Rf swig-3.0.12



