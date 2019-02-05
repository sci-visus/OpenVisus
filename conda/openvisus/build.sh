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
__cmake_options__+=" -DDISABLE_OPENMP=1"
__cmake_options__+=" -DVISUS_GUI=0"
__cmake_options__+=" -DVISUS_INTERNAL_DEFAULT=1"
__cmake_options__+=" -DPYTHON_VERSION=${PY_VER}"
__cmake_options__+=" -DPYTHON_EXECUTABLE=${PYTHON}"

if [ $(uname) = "Darwin" ]; then
	cmake -GXcode ${__cmake_options__} ${SOURCE_DIR}
	cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE}
	cmake --build ./ --target install   --config ${CMAKE_BUILD_TYPE} 	
else
	cmake ${__cmake_options__} -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${SOURCE_DIR}
	cmake --build ./ --target all -- -j 4
	cmake --build ./ --target install
fi

pushd install
$PYTHON setup.py install --single-version-externally-managed --record=record.txt
popd

set +ex
