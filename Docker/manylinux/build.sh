#!/bin/bash

# To build in docker:
# sudo docker build -t openvisus-manylinux .

# stop on errors printout commands
set -ex 

source "$(dirname $(readlink -f $0))/common.sh"

PushArg PYTHON_VERSION         3.6.6
PushArg CMAKE_BUILD_TYPE       RelWithDebugInfo
PushArg VISUS_INTERNAL_DEFAULT 1
PushArg DISABLE_OPENMP         1
PushArg VISUS_GUI              0
PushArg DEPS_INSTALL_DIR       $(pwd)/Linux-x86_64
PushArg DEPLOY_PYPI            0

yum update 
yum install -y zlib-devel curl 

InstallOpenSSLFromSource        $DEPS_INSTALL_DIR
InstallPyEnv                    $HOME
InstallPrecompiledCMakeForLinux $DEPS_INSTALL_DIR
InstallSwigFromSource           $DEPS_INSTALL_DIR

if ((VISUS_GUI==1)); then
	InstallQtForCentos5           $DEPS_INSTALL_DIR
fi

SetupCMakeOptions()
PushCmakeOption PYTHON_PLAT_NAME linux_x86_64)  
Build()










  
