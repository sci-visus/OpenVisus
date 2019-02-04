#!/bin/bash

set -ex 

source "$(dirname "$0")/build_utils.sh"

PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
VISUS_INTERNAL_DEFAULT=1
DISABLE_OPENMP=1
VISUS_GUI=0 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}
SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-$PWD/build} 

# directory for caching install stuff
CACHED_DIR=${BUILD_DIR}/cached_deps
mkdir -p ${CACHED_DIR}
export PATH=${CACHED_DIR}/bin:$PATH

yum update 
yum install -y zlib-devel curl  libffi-devel

InstallOpenSSL 
InstallPyEnvPython 
InstallCMake
InstallSwig

# for centos5 this is 2.2, I prefer to use 2.4 which is more common
# yum install -y httpd.x86_64 httpd-devel.x86_64
InstallApache24

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake_opts=""
PushCMakeOption APACHE_DIR             ${APACHE_DIR}
PushCMakeOption APR_DIR                ${APR_DIR}
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

cmake --build . --target all 
cmake --build . --target test
cmake --build . --target install 










  
