#!/bin/bash

PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}

# override
DISABLE_OPENMP=1
VISUS_GUI=0 
PLAT_NAME=manylinux1_x86_64

source "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

CACHED_DIR=${CACHED_DIR:$(pwd)/cached_deps}
mkdir -p ${CACHED_DIR}
export PATH=${CACHED_DIR}/bin:$PATH


# //////////////////////////////////////////////////////
function InstallCMake {

	if ! [ -x "${CACHED_DIR}/bin/cmake" ]; then
		echo "Downloading precompiled cmake"
		DownloadFile "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
		tar xzf cmake-3.4.3-Linux-x86_64.tar.gz  -C ${CACHED_DIR} --strip-components=1
	fi
}




# //////////////////////////////////////////////////////
function InstallSwig {

	if ! [ -x "${CACHED_DIR}/bin/swig" ]; then
		echo "Compiling swig"
		DownloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"  
		tar xzf swig-3.0.12.tar.gz 
		pushd swig-3.0.12 
		DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
		./Tools/pcre-build.sh 
		./configure --prefix=${CACHED_DIR}
		make -s -j 4 
		make install 
		popd
	fi
}

# //////////////////////////////////////////////////////
function InstallApache24 {

	APACHE_DIR=${CACHED_DIR}
	APR_DIR=${CACHED_DIR}

	if [ ! -f ${APACHE_DIR}/include/httpd.h ]; then
	
		echo "Compiling apache 2.4"	
	
		DownloadFile http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz
		tar xzf apr-1.6.5.tar.gz
		pushd apr-1.6.5
		./configure --prefix=${CACHED_DIR} && make -s && make install
		popd
		
		DownloadFile http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz
		tar xzf apr-util-1.6.1.tar.gz
		pushd apr-util-1.6.1
		./configure --prefix=${CACHED_DIR} --with-apr=${CACHED_DIR} && make -s && make install
		popd
		
		DownloadFile https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz
		tar xzf pcre-8.42.tar.gz
		pushd pcre-8.42
		./configure --prefix=${CACHED_DIR} && make -s && make install
		popd
		
		DownloadFile http://it.apache.contactlab.it/httpd/httpd-2.4.37.tar.gz
		tar xzf httpd-2.4.37.tar.gz
		pushd httpd-2.4.37
		./configure --prefix=${CACHED_DIR} --with-apr=${CACHED_DIR} --with-pcre=${CACHED_DIR} && make -s && make install
		popd
		
	fi
}

yum update 
yum install -y zlib-devel curl 

InstallPython 
InstallCMake
InstallSwig

# for centos5 this is 2.2, I prefer to use 2.4 which is more common
# yum install -y httpd.x86_64 httpd-devel.x86_64
InstallApache24

PushCMakeOption APACHE_DIR         ${APACHE_DIR}
PushCMakeOption APR_DIR            ${APR_DIR}
PushCMakeOptions

cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all 
cmake --build . --target test
cmake --build . --target install 

if (( DEPLOY_PYPI == 1 )); then
	cmake --build ./ --target pypi --config ${CMAKE_BUILD_TYPE}
fi








  
