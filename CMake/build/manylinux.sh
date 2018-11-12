#!/bin/bash

# To build in docker:
# sudo docker build -t openvisus-manylinux .

# stop on errors printout commands
set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-1} 
DISABLE_OPENMP=${DISABLE_OPENMP:-1} 
DEPS_INSTALL_DIR=${DEPS_INSTALL_DIR:-$(pwd)/linux_x86_64} 
PYTHON_PLAT_NAME=linux_x86_64
DEPLOY_PYPI=${DEPLOY_PYPI:-0} 

source "$(dirname $(readlink -f $0))/common.sh"

yum update 
yum install -y zlib-devel curl 

InstallOpenSSL 
InstallPython
InstallCMake    
InstallSwig     

# Gui not working right now
# InstallQtForCentos5  

SetupOpenVisusCMakeOptions 
BuildOpenVisus












  
