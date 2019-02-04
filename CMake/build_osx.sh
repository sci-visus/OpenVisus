#!/bin/bash

set -ex 

source "$(dirname "$0")/build_utils.sh"

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6}
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

InstallBrew
InstallPyEnvPython

brew install swig  
if (( VISUS_INTERNAL_DEFAULT == 0 )); then 
	brew install zlib lz4 tinyxml freeimage openssl curl
fi

if (( VISUS_GUI == 1 )); then
	# install qt 5.11 (instead of 5.12 which is not supported by PyQt5)
	# brew install qt5
	brew unlink git || true
	brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb
	Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5	
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
cmake -GXcode ${cmake_opts} ${SOURCE_DIR} 

# this is to solve logs too long 
if [[ "${TRAVIS_OS_NAME}" == "osx" ]] ; then
	sudo gem install xcpretty  
	set -o pipefail && cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
else
	cmake --build ./ --target ALL_BUILD   --config ${CMAKE_BUILD_TYPE}
fi

cmake --build ./ --target RUN_TESTS   --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target install     --config ${CMAKE_BUILD_TYPE} 




