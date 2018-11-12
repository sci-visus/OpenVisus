#!/bin/bash

set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6.5} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
VISUS_GUI=${VISUS_GUI:-1}  

source "$(dirname $0)/common.sh"

InstallBrew            
InstallPython 

# this is to solve logs too long 
gem install xcpretty   

brew install swig  
if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  brew install zlib lz4 tinyxml freeimage openssl curl
fi

if (( VISUS_GUI==1 )); then
	InstallQt
fi

SetupOpenVisusCMakeOptions
BuildOpenVisus









