#!/bin/bash

set -ex

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

echo "SOURCE_DIR        ${SOURCE_DIR}"
echo "BUILD_DIR         ${BUILD_DIR}"
echo "CMAKE_BUILD_TYPE  ${CMAKE_BUILD_TYPE}"
echo "PY_VER            ${PY_VER}"
echo "PYTHON            ${PYTHON}"

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

__cmake_options__=""

# XCode for Darwin doesn't work
if [ $(uname) = "Darwin" ]; then
	__cmake_options__+=" -DCMAKE_C_COMPILER=clang"
	__cmake_options__+=" -DCMAKE_CXX_COMPILER=clang++"
fi

__cmake_options__+=" -DDISABLE_OPENMP=1"
__cmake_options__+=" -DVISUS_GUI=0"
__cmake_options__+=" -DVISUS_INTERNAL_DEFAULT=1"
__cmake_options__+=" -DPYTHON_VERSION=${PY_VER}"
__cmake_options__+=" -DPYTHON_EXECUTABLE=${PYTHON}"

# having problems with some samples...
__cmake_options__+=" -DVISUS_TEST=OFF"

cmake ${__cmake_options__} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${SOURCE_DIR}

cmake --build ./ --target all -- -j 4

cmake --build ./ --target install  

pushd install
$PYTHON setup.py install --single-version-externally-managed --record=record.txt
popd

set +ex
