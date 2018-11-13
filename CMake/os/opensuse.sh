#!/bin/bash

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
VISUS_GUI=${VISUS_GUI:-1} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 
BUILD_DIR=${BUILD_DIR:-/tmp/OpenVisus/build} 

# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	zypper --non-interactive update
	zypper --non-interactive install sudo	
fi

sudo zypper --non-interactive update
sudo zypper --non-interactive install --type pattern devel_basis
sudo zypper --non-interactive install gcc-c++ cmake git swig  libuuid-devel libopenssl-devel curl patchelf
sudo zypper --non-interactive install apache2 apache2-devel

if ((VISUS_INTERNAL_DEFAULT==0)); then
	sudo zypper --non-interactive install zlib-devel liblz4-devel tinyxml-devel libfreeimage-devel libcurl-devel
fi

InstallPython

if (( VISUS_GUI==1 )); then
	InstallQt
fi

ConfigureOpenVisus
BuildOpenVisus


