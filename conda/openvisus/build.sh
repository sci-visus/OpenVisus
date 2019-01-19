#/usr/bin/env bash

set -ex 

CMAKE_BUILD_TYPE=RelWithDebInfo

mkdir -p build
cd build

cmake \
	-DPYTHON_EXECUTABLE=$PYTHON \
	-DPYTHON_VERSION=$PY_VER \
	-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE \
	-DDISABLE_OPENMP=1 \
	-DVISUS_GUI=0 \
	-DVISUS_INTERNAL_DEFAULT=1 \
	-DVISUS_INTERNAL_OPENSSL=0 \
	-DVISUS_INTERNAL_CURL=0 \
	-DCMAKE_INSTALL_PREFIX=$PREFIX \
	..

cmake --build . --target all -- -j 4

python \
	setup.py install \
	--prefix=${PREFIX} \
	--single-version-externally-managed \
	--record=record.txt
