#!/bin/bash

set -x # very verbose
set -e # stop on error

curl -L --insecure --retry 3  "http://download.qt.io/official_releases/qt/5.9/5.9.4/single/qt-everywhere-opensource-src-5.9.4.tar.xz" -O

tar xvf qt-everywhere-opensource-src-5.9.4.tar.xz

rm qt-everywhere-opensource-src-5.9.4.tar.xz

pushd qt-everywhere-opensource-src-5.9.4

sed -i "s/#define QTESTLIB_USE_PERF_EVENTS/#undef QTESTLIB_USE_PERF_EVENTS/g" qtbase/src/testlib/qbenchmark_p.h 

./configure \
	-prefix /opt/qt59 \
	-openssl \
	-opensource \
	-nomake examples \
	-nomake tests \
	-opengl desktop \
	-confirm-license \
	-fontconfig \
	-system-freetype \
	-D _X_INLINE=inline \
	-D XK_dead_currency=0xfe6f \
	-D XK_ISO_Level5_Lock=0xfe13 \
	-D FC_WEIGHT_EXTRABLACK=215 \
	-D FC_WEIGHT_ULTRABLACK=FC_WEIGHT_EXTRABLACK  \
	-D GLX_GLXEXT_PROTOTYPES=On \
	-skip activeqt \
	-skip androidextras \
	-skip connectivity \
	-skip enginio \
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

rm -Rf qt-everywhere-opensource-src-5.9.4

