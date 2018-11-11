#!/bin/bash

# stop on errors printout commands
set -ex 

source "$(dirname $(readlink -f $0))/common.sh"

PushArg PYTHON_VERSION         3.6.6
PushArg CMAKE_BUILD_TYPE       RelWithDebugInfo
PushArg VISUS_INTERNAL_DEFAULT 0
PushArg DISABLE_OPENMP         0
PushArg VISUS_GUI              1
PushArg DEPS_INSTALL_DIR       $(pwd)/osx
PushArg DEPLOY_PYPI            0
            
InstallPython $PYTHON_VERSION

# this is to solve logs too long 
gem install xcpretty   

#  install dependencies using brew
brew install swig  
if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  brew install zlib lz4 tinyxml freeimage openssl curl
fi

brew install qt5
Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5

SetupOpenVisusCMakeOptions

mkdir build
cd build
cmake -GXcode ${cmake_opts} ../ 

set -o pipefail && \
cmake --build ./ --target ALL_BUILD   --config ${CMAKE_BUILD_TYPE} | xcpretty -c
cmake --build ./ --target RUN_TESTS   --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target install     --config ${CMAKE_BUILD_TYPE}  
cmake --build ./ --target deploy      --config ${CMAKE_BUILD_TYPE} 
cmake --build ./ --target bdist_wheel --config ${CMAKE_BUILD_TYPE} 
cmake --build ./ --target sdist       --config ${CMAKE_BUILD_TYPE} 

if ((DEPLOY_PYPI==1)); then 
  cmake --build ./ --target pypi      --config ${CMAKE_BUILD_TYPE}
fi








