#!/bin/bash

# stop on errors printout commands
set -ex 

source "$(dirname $(readlink -f $0))/../common.sh"

PushArg PYTHON_VERSION         3.6.6
PushArg CMAKE_BUILD_TYPE       RelWithDebugInfo
PushArg VISUS_INTERNAL_DEFAULT 0
PushArg DISABLE_OPENMP         0
PushArg VISUS_GUI              1
PushArg DEPS_INSTALL_DIR       $(pwd)/osx
PushArg DEPLOY_PYPI            0
            
InstallPyEnv $HOME

# this is to solve logs too long 
gem install xcpretty   

#  install dependencies using brew
brew install swig  
if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  brew install zlib lz4 tinyxml freeimage openssl curl
fi

brew install qt5
Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5

SetupCMakeOptions()
Build()






