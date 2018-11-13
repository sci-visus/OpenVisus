#!/bin/bash

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
VISUS_INTERNAL_DEFAULT=1
DISABLE_OPENMP=1
VISUS_GUI=0
PYTHON_PLAT_NAME=linux_x86_64
CMAKE_BUILD_TYPE=Release 
BUILD_DIR=${BUILD_DIR:-/tmp/OpenVisus/build} 

yum update 
yum install -y zlib-devel curl 
yum install -y httpd.x86_64 httpd-devel.x86_64

InstallOpenSSL 
InstallPython
InstallCMake    
InstallSwig

# broken right now
# InstallQt  

ConfigureOpenVisus 
BuildOpenVisus












  
