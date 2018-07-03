#!/bin/bash

INSTALL_DIR=$1

set -e
curl https://datapacket.dl.sourceforge.net/project/freeimage/Source%20Distribution/3.16.0/FreeImage3160.zip -O 
unzip FreeImage3160.zip && cd FreeImage 
CXX="g++ -Wno-narrowing" make -j 4 
cp Dist/*.h $INSTALL_DIR/include && cp Dist/*.so $INSTALL_DIR/lib 

cd ../ && rm -Rf FreeImage




