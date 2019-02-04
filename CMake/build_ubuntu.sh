#!/bin/bash

set -ex 

source "$(dirname "$0")/build_utils.sh"

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6}
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}
SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-$PWD/build} 

# directory for caching install stuff
CACHED_DIR=${BUILD_DIR}/cached_deps
mkdir -p ${CACHED_DIR}
export PATH=${CACHED_DIR}/bin:$PATH

# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	apt-get -qq update
	apt-get -qq install sudo	
fi

sudo apt-get -qq update
sudo apt-get -qq install git software-properties-common

DetectUbuntuVersion

if (( ${OS_VERSION:0:2}<=14 )); then
	sudo add-apt-repository -y ppa:deadsnakes/ppa
	sudo apt-get -qq update
fi

sudo apt-get -qq install --allow-unauthenticated cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev uuid-dev curl automake
sudo apt-get -qq install --allow-unauthenticated libffi-dev   
sudo apt-get -qq install --allow-unauthenticated apache2 apache2-dev

InstallCMake
InstallPatchElf
InstallPyEnvPython 

if (( VISUS_INTERNAL_DEFAULT == 0 )); then 
	sudo apt-get -qq install --allow-unauthenticated zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

# install qt (it's a version available on PyQt5)
if (( VISUS_GUI==1 )); then

	# https://launchpad.net/~beineri
	# PyQt5 versions 5.6, 5.7, 5.7.1, 5.8, 5.8.1.1, 5.8.2, 5.9, 5.9.1, 5.9.2, 5.10, 5.10.1, 5.11.2, 5.11.3
	if (( ${OS_VERSION:0:2} <=14 )); then
		
		sudo add-apt-repository ppa:beineri/opt-qt-5.10.1-trusty -y
		sudo apt-get -qq update 
		sudo apt-get install -yqq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt510base 
		set +e 
		source /opt/qt510/bin/qt510-env.sh
		set -e 

	elif (( ${OS_VERSION:0:2} <=16 )); then

		sudo add-apt-repository ppa:beineri/opt-qt-5.11.2-xenial -y
		sudo apt-get -qq update 
		sudo apt-get install -yqq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt511base
		set +e 
		source /opt/qt511/bin/qt511-env.sh
		set -e 

	elif (( ${OS_VERSION:0:2} <=18)); then

		sudo add-apt-repository ppa:beineri/opt-qt-5.11.2-bionic -y
		sudo apt-get -qq update 
		sudo apt-get install -yqq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt511base
		set +e 
		source /opt/qt511/bin/qt511-env.sh
		set -e 
	fi

	export Qt5_DIR=${QTDIR}/lib/cmake/Qt5
fi

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake_opts=""
PushCMakeOption PYTHON_VERSION         ${PYTHON_VERSION}
PushCMakeOption VISUS_INTERNAL_DEFAULT ${VISUS_INTERNAL_DEFAULT}
PushCMakeOption DISABLE_OPENMP         ${DISABLE_OPENMP}
PushCMakeOption VISUS_GUI              ${VISUS_GUI}
PushCMakeOption CMAKE_BUILD_TYPE       ${CMAKE_BUILD_TYPE}
PushCMakeOption PYTHON_EXECUTABLE      ${PYTHON_EXECUTABLE}
PushCMakeOption PYTHON_INCLUDE_DIR     ${PYTHON_INCLUDE_DIR}
PushCMakeOption PYTHON_LIBRARY         ${PYTHON_LIBRARY}
PushCMakeOption Qt5_DIR                ${Qt5_DIR}
PushCMakeOption SWIG_EXECUTABLE        $(which swig3.0)
cmake ${cmake_opts} ${SOURCE_DIR} 
 
cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install



