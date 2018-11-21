#!/bin/bash

. "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

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

cmake_opts=""
cmake_opts+=" -GXcode"
PushCMakeOptions
cmake ${cmake_opts} ${SOURCE_DIR} 

set -o pipefail && \
cmake --build ./ --target ALL_BUILD   --config ${CMAKE_BUILD_TYPE} | xcpretty -c
cmake --build ./ --target RUN_TESTS   --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target install     --config ${CMAKE_BUILD_TYPE}  
cmake --build ./ --target deploy      --config ${CMAKE_BUILD_TYPE} 

pushd install
PYTHONPATH=$(pwd) bin/visus.app/Contents/MacOS/visus  && echo "Embedding working"  
PYTHONPATH=$(pwd) python -c "import OpenVisus"        && echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi

if (( DEPLOY_PYPI == 1 )); then
	cmake --build ./ --target bdist_wheel --config ${CMAKE_BUILD_TYPE} 
	cmake --build ./ --target pypi        --config ${CMAKE_BUILD_TYPE}
fi




