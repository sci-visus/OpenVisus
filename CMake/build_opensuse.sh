#!/bin/bash

. "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	zypper --non-interactive update
	zypper --non-interactive install sudo	
fi

sudo zypper --non-interactive update
sudo zypper --non-interactive install --type pattern devel_basis
sudo zypper --non-interactive install gcc-c++ cmake git swig libuuid-devel curl patchelf
sudo zypper --non-interactive install apache2 apache2-devel


# //////////////////////////////////////////////////////
function InstallQt {

	sudo zypper addrepo http://widehat.opensuse.org/opensuse/repositories/KDE:/Qt5/openSUSE_Leap_42.3/ kde-5
	sudo zypper -n in glu-devel libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel

	#Qt5_DIR=${CACHED_DIR}/Qt/${QT5_VERSION}/gcc_64/lib/cmake/Qt5
	#if [ -d "${Qt5_DIR}" ]; then
	#	mkdir ${CACHED_DIR}/Qt
	#	pushd ${CACHED_DIR}/Qt
	#	sudo zypper in p7zip wget python3-requests  
	#	wget
	#	chmod +x ./qli-installer.py
	#	./qli-installer.py ${QT5_VERSION} linux desktop
	#	export 
	#	popd
	#fi
}


InstallPython 

if (( VISUS_GUI==1 )); then
	InstallQt
fi

PushCMakeOptions
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 


