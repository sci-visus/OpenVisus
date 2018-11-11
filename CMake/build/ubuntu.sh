#!/bin/bash

# stop on errors printout commands
set -ex 

# configuration
PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
DISABLE_OPENMP=${DISABLE_OPENMP:-0} 
VISUS_GUI=${VISUS_GUI:-1} 
DEPS_INSTALL_DIR=${s:-$(pwd)/Linux-x86_64} 
DEPLOY_PYPI=${DEPLOY_PYPI:-0} 
VISUS_MODVISUS=${VISUS_MODVISUS:-0} 

if ((VISUS_MODVISUS==1)); then
	CMAKE_INSTALL_PREFIX=/home/visus
	VISUS_HOME=$CMAKE_INSTALL_PREFIX}
	VISUS_PYTHON_SYS_PATH=$CMAKE_INSTALL_PREFIX}
fi

source "$(dirname $(readlink -f $0))/common.sh"

#  install linux dependencies
sudo apt-get -qy install software-properties-common
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get -qy update
sudo apt-get -qy install --allow-unauthenticated cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev uuid-dev curl

SWIG_EXECUTABLE=$(which swig3.0)

if ((VISUS_MODVISUS==1)); then
	sudo apt-get -qy install --allow-unauthenticated apache2 apache2-dev 
fi

InstallPatchElf 
InstallPython   
InstallCMake    

if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  sudo apt-get -qy install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

if ((VISUS_GUI==1)); then
	InstallQtForUbuntu 
fi

SetupOpenVisusCMakeOptions
BuildOpenVisus

if ((VISUS_MODVISUS==1)); then
	InstallModVisus
fi





