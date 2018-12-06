#!/bin/bash

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
}


InstallBrew            
InstallPython

# this is to solve logs too long 
gem install xcpretty   

brew install swig  

if (( VISUS_INTERNAL_DEFAULT == 0 )); then 
  brew install zlib lz4 tinyxml freeimage openssl curl
fi

if (( VISUS_GUI == 1 )); then
	brew install qt5
	Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5	
fi

PushCMakeOptions
cmake -GXcode ${cmake_opts} ${SOURCE_DIR} 

set -o pipefail && \
cmake --build ./ --target ALL_BUILD   --config ${CMAKE_BUILD_TYPE} | xcpretty -c
cmake --build ./ --target RUN_TESTS   --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target install     --config ${CMAKE_BUILD_TYPE}  
cmake --build ./ --target deploy      --config ${CMAKE_BUILD_TYPE} 

pushd install
PYTHONPATH=$(pwd) bin/visus.app/Contents/MacOS/visus  && echo "Embedding working"  
PYTHONPATH=$(pwd) python -c "import VisusKernelPy"        && echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi

if (( DEPLOY_PYPI == 1 )); then
	cmake --build ./ --target bdist_wheel --config ${CMAKE_BUILD_TYPE} 
	cmake --build ./ --target pypi        --config ${CMAKE_BUILD_TYPE}
fi




