#!/bin/bash

# stop on errors printout commands
set -ex 

# configuration
PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
DISABLE_OPENMP=${DISABLE_OPENMP:-0} 
VISUS_GUI=${VISUS_GUI:-1} 
DEPS_INSTALL_DIR=${s:-$(pwd)/Linux-osx} 
DEPLOY_PYPI=${DEPLOY_PYPI:-0} 

source "$(dirname $(readlink -f $0))/common.sh"
            
InstallPython 

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








