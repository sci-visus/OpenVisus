#!/bin/bash

set -x # very verbose
set -e # stop on error

curl -L --insecure --retry 3 "http://download.qt.io/official_releases/qt/5.12/5.12.8/single/qt-everywhere-src-5.12.8.tar.xz" -O		

tar xvf qt-everywhere-src-5.12.8.tar.xz

rm qt-everywhere-src-5.12.8.tar.xz

pushd qt-everywhere-src-5.12.8

./configure \
	-prefix /opt/qt512 \
	-openssl \
	-opensource \
	-no-use-gold-linker \
	-nomake examples \
	-nomake tests \
	-opengl desktop \
	-confirm-license \
	-fontconfig \
	-system-freetype \
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

rm -Rf qt-everywhere-src-5.12.8

