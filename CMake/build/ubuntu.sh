#!/bin/bash

set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
VISUS_GUI=${VISUS_GUI:-1} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 

source "$(dirname $(readlink -f $0))/common.sh"

if ((${UBUNTU_VERSION:0:2}<=14)); then
	sudo apt-get -qq install software-properties-common
	sudo add-apt-repository -y ppa:deadsnakes/ppa
fi

sudo apt-get -qq update
sudo apt-get -qq install --allow-unauthenticated cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev uuid-dev curl
sudo apt-get -qq install apache2 apache2-dev

SWIG_EXECUTABLE=$(which swig3.0)

InstallPatchElf 
InstallPython   
InstallCMake    

if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  sudo apt-get -qq install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

if (( VISUS_GUI==1 )); then
	InstallQt
fi

SetupOpenVisusCMakeOptions
BuildOpenVisus





