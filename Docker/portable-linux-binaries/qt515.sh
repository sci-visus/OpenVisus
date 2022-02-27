#!/bin/bash

set -x # very verbose
set -e # stop on error

VERSION=5.15.2
DIR=/opt/qt515
curl -L --insecure --retry 3 "http://download.qt.io/official_releases/qt/${VERSION:0:4}/$VERSION/single/qt-everywhere-src-$VERSION.tar.xz" -O		

tar xf qt-everywhere-src-$VERSION.tar.xz 

rm -f qt-everywhere-src-$VERSION.tar.xz

pushd qt-everywhere-src-$VERSION

./configure \
	-prefix $DIR \
	-opensource \
	-nomake examples \
	-nomake tests \
	-opengl desktop \
	-confirm-license \
	-skip activeqt \
	-skip androidextras \
	-skip connectivity \
	-skip location \
	-skip macextras \
	-skip multimedia \
	-skip sensors \
	-skip serialport \
	-skip wayland \
	-skip webchannel \
	-skip websockets \
	-skip winextras \
	-skip x11extras \
	-skip qtgamepad 

make 
make install 

popd

rm -Rf qt-everywhere-src-$VERSION

