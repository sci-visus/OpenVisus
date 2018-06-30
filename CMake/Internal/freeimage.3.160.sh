#!/bin/bash

set -e

INSTALL_DIR=$1

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp
curl https://datapacket.dl.sourceforge.net/project/freeimage/Source%20Distribution/3.16.0/FreeImage3160.zip -O 
unzip FreeImage3160.zip && cd FreeImage
make CXX="g++ -Wno-narrowing" -j 4
cp Dist/*.h   $INSTALL_DIR/include && cp Dist/*.so  $INSTALL_DIR/lib

cd /tmp 
rm -Rf FreeImage

