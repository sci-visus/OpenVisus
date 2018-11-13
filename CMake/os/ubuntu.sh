#!/bin/bash

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
VISUS_GUI=${VISUS_GUI:-1} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
BUILD_DIR=${BUILD_DIR:-/tmp/OpenVisus/build} 

# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	apt-get -qq update
	apt-get -qq install sudo	
fi

sudo apt-get -qq update
sudo apt-get -qq install git 

if (( ${OS_VERSION:0:2}<=14 )); then
	sudo apt-get -qq install software-properties-common
	sudo add-apt-repository -y ppa:deadsnakes/ppa
	sudo apt-get -qq update
fi

sudo apt-get -qq install --allow-unauthenticated cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev uuid-dev curl automake
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

ConfigureOpenVisus
BuildOpenVisus



