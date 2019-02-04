#!/bin/bash

set -ex

source "./build_utils.sh"

SOURCE_DIR=$(pwd)

PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-1}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}
BUILD_DIR=${BUILD_DIR:-$PWD/build} 

# ___________________________________________________ docker
if [ -n "${DOCKER_IMAGE}" ]; then
	
	sudo docker rm -f mydocker 2>/dev/null || true
	
	sudo docker run -d -ti --name mydocker -v ${SOURCE_DIR}:${SOURCE_DIR} \
			-e PYTHON_VERSION=${PYTHON_VERSION} \
			-e VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
			-e DISABLE_OPENMP=${DISABLE_OPENMP} \
			-e VISUS_GUI=${VISUS_GUI} \
			-e CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
			-e BUILD_DIR=${BUILD_DIR} \
		${DOCKER_IMAGE} /bin/bash
		
	sudo docker exec mydocker /bin/bash -c "cd ${SOURCE_DIR} && ./build.sh"

	# fix permissions
	sudo chown -R "$USER":"$USER" ${BUILD_DIR}
	sudo chmod -R u+rwx           ${BUILD_DIR}
	
	exit 0
	
	
# directory for caching install stuff
CACHED_DIR=${BUILD_DIR}/cached_deps
mkdir -p ${CACHED_DIR}
export PATH=${CACHED_DIR}/bin:$PATH

# ___________________________________________________ osx
if (( OSX == 1 )); then 

	InstallBrew

	brew install swig  
	if (( VISUS_INTERNAL_DEFAULT == 0 )); then 
		brew install zlib lz4 tinyxml freeimage openssl curl
	fi

	if (( VISUS_GUI == 1 )); then
		# install qt 5.11 (instead of 5.12 which is not supported by PyQt5)
		# brew install qt5
		brew unlink git || true
		brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb
		Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5	
	fi


# ___________________________________________________ ubuntu
elif (( UBUNTU == 1 )); then 

	# make sure sudo is available
	if [ "$EUID" -eq 0 ]; then
		apt-get -qq update
		apt-get -qq install sudo	
	fi

	sudo apt-get -qq update
	sudo apt-get -qq install git software-properties-common

	if (( ${OS_VERSION:0:2}<=14 )); then
		sudo add-apt-repository -y ppa:deadsnakes/ppa
		sudo apt-get -qq update
	fi

	sudo apt-get -qq install --allow-unauthenticated cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev uuid-dev curl automake
	sudo apt-get -qq install --allow-unauthenticated libffi-dev   
	sudo apt-get -qq install --allow-unauthenticated apache2 apache2-dev

	InstallCMake
	InstallPatchElf

	if (( VISUS_INTERNAL_DEFAULT == 0 )); then 
		sudo apt-get -qq install --allow-unauthenticated zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
	fi

	# install qt (it's a version available on PyQt5)
	if (( VISUS_GUI == 1 )); then
		InstallQt5ForUbuntu
	fi

	export SWIG_EXECUTABLE=$(which swig3.0)

# ___________________________________________________ opensuse
elif (( OPENSUSE == 1 )); then 

	# make sure sudo is available
	if [ "$EUID" -eq 0 ]; then
		zypper --non-interactive update
		zypper --non-interactive install sudo	
	fi

	sudo zypper --non-interactive update
	sudo zypper --non-interactive install --type pattern devel_basis
	sudo zypper --non-interactive install lsb-release gcc-c++ cmake git swig  libuuid-devel libopenssl-devel curl patchelf
	sudo zypper --non-interactive install apache2 apache2-devel
	sudo zypper --non-interactive install libffi-devel

	if (( VISUS_INTERNAL_DEFAULT == 0 )); then
		sudo zypper --non-interactive install zlib-devel liblz4-devel tinyxml-devel libfreeimage-devel libcurl-devel
	fi

	if (( VISUS_GUI==1 )); then
		sudo zypper -n in  glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel
	fi

# ___________________________________________________ opensuse
elif (( MANYLINUX == 1 )); then 

	# override
	VISUS_INTERNAL_DEFAULT=1
	DISABLE_OPENMP=1
	VISUS_GUI=0 

	yum update 
	yum install -y zlib-devel curl  libffi-devel

	InstallOpenSSL 
	InstallCMake
	InstallSwig

	# for centos5 this is 2.2, I prefer to use 2.4 which is more common
	# yum install -y httpd.x86_64 httpd-devel.x86_64
	InstallApache24

else
	echo "Failed to detect OS version"
fi

InstallPyEnvPython

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
PushCMakeOption SWIG_EXECUTABLE        ${SWIG_EXECUTABLE}
PushCMakeOption APACHE_DIR             ${APACHE_DIR}
PushCMakeOption APR_DIR                ${APR_DIR}	

if (( OSX == 1 )) ; then
	cmake -GXcode ${cmake_opts} ${SOURCE_DIR} 
	
	# this is to solve logs too long 
	if [[ "${TRAVIS_OS_NAME}" == "osx" ]] ; then
		sudo gem install xcpretty  
		set -o pipefail && cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
	else
		cmake --build ./ --target ALL_BUILD   --config ${CMAKE_BUILD_TYPE}
	fi

	cmake --build ./ --target RUN_TESTS   --config ${CMAKE_BUILD_TYPE}
	cmake --build ./ --target install     --config ${CMAKE_BUILD_TYPE} 		
	
else
	cmake ${cmake_opts} ${SOURCE_DIR} 
	cmake --build . --target all -- -j 4
	cmake --build . --target test
	cmake --build . --target install	
fi 
 
