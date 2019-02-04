#!/bin/bash

set -ex 

source "$(dirname "$0")/build_utils.sh"

PYTHON_VERSION=${PYTHON_VERSION:-3.6}
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}
SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-$PWD/build} 

# directory for caching install stuff
CACHED_DIR=${BUILD_DIR}/cached_deps
mkdir -p ${CACHED_DIR}
export PATH=${CACHED_DIR}/bin:$PATH

# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	zypper --non-interactive update
	zypper --non-interactive install sudo	
fi

sudo zypper --non-interactive update
sudo zypper --non-interactive install --type pattern devel_basis
sudo zypper --non-interactive install lsb-release gcc-c++ cmake git swig  libuuid-devel libopenssl-devel curl patchelf
sudo zypper --non-interactive install apache2 apache2-devel
sudo zypper --non-interactive install libffi-devel

if (( VISUS_INTERNAL_DEFAULT == 0 )); then
	sudo zypper --non-interactive install zlib-devel liblz4-devel tinyxml-devel libfreeimage-devel libcurl-devel
fi

InstallPyEnvPython 

if (( VISUS_GUI==1 )); then
	sudo zypper -n in  glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake_opts=""
PushCMakeOption PYTHON_VERSION         ${PYTHON_VERSION}
PushCMakeOption VISUS_INTERNAL_DEFAULT ${VISUS_INTERNAL_DEFAULT}
PushCMakeOption DISABLE_OPENMP         ${DISABLE_OPENMP}
PushCMakeOption VISUS_GUI              ${VISUS_GUI}
PushCMakeOption CMAKE_BUILD_TYPE       ${CMAKE_BUILD_TYPE}
PushCMakeOption PYTHON_EXECUTABLE      ${PYTHON_EXECUTABLE}
PushCMakeOption PYTHON_INCLUDE_DIR     ${PYTHON_INCLUDE_DIR}
PushCMakeOption PYTHON_LIBRARY         ${PYTHON_LIBRARY}
PushCMakeOption Qt5_DIR                ${Qt5_DIR}
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 


