#!/bin/bash

set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 

VISUS_INTERNAL_DEFAULT=1
DISABLE_OPENMP=1
PYTHON_PLAT_NAME=linux_x86_64
CMAKE_BUILD_TYPE=Release 

source "$(dirname $(readlink -f $0))/common.sh"

yum update 
yum install -y zlib-devel curl 
yum install -y httpd.x86_64 httpd-devel.x86_64

InstallOpenSSL 
InstallPython
InstallCMake    
InstallSwig
# InstallQt  

SetupOpenVisusCMakeOptions 
BuildOpenVisus












  
