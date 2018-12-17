#!/bin/bash

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6}
IS_TRAVIS=${IS_TRAVIS:0}
source "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# //////////////////////////////////////////////////////
function InstallBrew {

	if ! [ -x "$(command -v brew)" ]; then
		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	fi
	
	# output is very long!
	brew update 1>/dev/null 2>&1 || true
	brew upgrade pyenv 1>/dev/null 2>&1 || true
}


InstallBrew            
InstallPython

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

PushCMakeOptions
cmake -GXcode ${cmake_opts} ${SOURCE_DIR} 

# this is to solve logs too long 
if (( IS_TRAVIS == 1 )) ; then
	sudo gem install xcpretty  
	set -o pipefail && cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
else
	cmake --build ./ --target ALL_BUILD   --config ${CMAKE_BUILD_TYPE}
fi

cmake --build ./ --target RUN_TESTS   --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target install     --config ${CMAKE_BUILD_TYPE} 

if (( DEPLOY_PYPI == 1 )); then
	cmake --build ./ --target pypi     --config ${CMAKE_BUILD_TYPE}
fi



