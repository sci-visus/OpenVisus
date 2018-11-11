#!/bin/bash


# stop on errors printout commands
set -ex 

source "./CMake/common.sh"

PushArg PYTHON_VERSION         3.6.6
PushArg CMAKE_BUILD_TYPE       RelWithDebugInfo
PushArg VISUS_INTERNAL_DEFAULT 0
PushArg DISABLE_OPENMP         0
PushArg VISUS_GUI              1
PushArg DEPS_INSTALL_DIR       $(pwd)/Linux-x86_64
PushArg DEPLOY_PYPI            0

#  install linux dependencies 
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get -qy update
sudo apt-get -qy install --allow-unauthenticated cmake swig git bzip2 ca-certificates build-essential libssl-dev uuid-dev
sudo apt-get -qy install --allow-unauthenticated apache2 apache2-dev 

InstallPyEnv                    $HOME
InstallPatchElfFromSource       $DEPS_INSTALL_DIR
InstallPrecompiledCMakeForLinux $DEPS_INSTALL_DIR

# install dependencies
if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  sudo apt-get install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

# install qt 
if ((VISUS_GUI==1)); then
	InstallQtForUbuntu 
fi

SetupCmakeOptions()
Build()






