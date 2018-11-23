#!/bin/sh

set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6}
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
CMAKE_BUILD_TYPE=RelWithDebInfo 

DEPLOY_GITHUB=${DEPLOY_GITHUB:-0}

DEPLOY_PYPI=${DEPLOY_PYPI:-0}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_PASSWORD=${PYPI_PASSWORD:-}
PYPI_PLAT_NAME=${PYPI_PLAT_NAME:-}

BUILD_DIR=${BUILD_DIR:-$(pwd)/build} 

cmake_opts=""

# //////////////////////////////////////////////////////
function DownloadFile {
	curl -fsSL --insecure "$1" -O
}

# //////////////////////////////////////////////////////
function PushCMakeOption {
	if [ -n "$2" ] ; then
		cmake_opts="${cmake_opts} -D$1=$2"
	fi
}

function PushDockerEnv {
	if [ -n "$2" ] ; then
		docker_env+=" -e $1=$2"
	fi
}

# //////////////////////////////////////////////////////
function PushCMakeOptions {

	PushCMakeOption PYTHON_VERSION         ${PYTHON_VERSION}
	PushCMakeOption VISUS_INTERNAL_DEFAULT ${VISUS_INTERNAL_DEFAULT}
	PushCMakeOption DISABLE_OPENMP         ${DISABLE_OPENMP}
	PushCMakeOption VISUS_GUI              ${VISUS_GUI}
	PushCMakeOption CMAKE_BUILD_TYPE       ${CMAKE_BUILD_TYPE}
	
	PushCMakeOption PYPI_USERNAME          ${PYPI_USERNAME}
	PushCMakeOption PYPI_PASSWORD          ${PYPI_PASSWORD}
	PushCMakeOption PYPI_PLAT_NAME         ${PYPI_PLAT_NAME}
	
	PushCMakeOption OPENSSL_ROOT_DIR       ${OPENSSL_ROOT_DIR}
	
	PushCMakeOption PYTHON_EXECUTABLE      ${PYTHON_EXECUTABLE}
	PushCMakeOption PYTHON_INCLUDE_DIR     ${PYTHON_INCLUDE_DIR}
	PushCMakeOption PYTHON_LIBRARY         ${PYTHON_LIBRARY}
	
	PushCMakeOption Qt5_DIR                ${Qt5_DIR}
}

# //////////////////////////////////////////////////////
function PushDockerEnvs {

	PushDockerEnv PYTHON_VERSION          ${PYTHON_VERSION}
	PushDockerEnv VISUS_INTERNAL_DEFAULT  ${VISUS_INTERNAL_DEFAULT}
	PushDockerEnv DISABLE_OPENMP          ${DISABLE_OPENMP}
	PushDockerEnv VISUS_GUI               ${VISUS_GUI}
	PushDockerEnv CMAKE_BUILD_TYPE        ${CMAKE_BUILD_TYPE}
	
	PushDockerEnv DEPLOY_GITHUB           ${DEPLOY_GITHUB}
	
	PushDockerEnv DEPLOY_PYPI             ${DEPLOY_PYPI}
	PushDockerEnv PYPI_USERNAME           ${PYPI_USERNAME}
	PushDockerEnv PYPI_PASSWORD           ${PYPI_PASSWORD}
	PushDockerEnv PYPI_PLAT_NAME          ${PYPI_PLAT_NAME}
	
	PushDockerEnv BUILD_DIR               ${BUILD_DIR}
}


# //////////////////////////////////////////////////////
function InstallPython {

   # need to install pyenv?
	if ! [ -x "$(command -v pyenv)" ]; then
		DownloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
		chmod a+x pyenv-installer 
		./pyenv-installer 
		rm -f pyenv-installer 
	fi
	
	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"
	eval "$(pyenv virtualenv-init -)"
	
	# pyenv install --list
	
	if [ -n "${OPENSSL_INCLUDE_DIR}" ]; then
		CONFIGURE_OPTS=--enable-shared CFLAGS=-I${OPENSSL_INCLUDE_DIR} CPPFLAGS=-I${OPENSSL_INCLUDE_DIR}/ LDFLAGS=-L${OPENSSL_LIB_DIR} pyenv install --skip-existing  ${PYTHON_VERSION}  
	else
		CONFIGURE_OPTS=--enable-shared pyenv install --skip-existing ${PYTHON_VERSION}  
	fi
	
	pyenv global ${PYTHON_VERSION}  
	pyenv rehash
	python -m pip install --upgrade pip  
	python -m pip install numpy setuptools wheel twine auditwheel 

	if [ "${PYTHON_VERSION:0:1}" -gt "2" ]; then
		PYTHON_M_VERSION=${PYTHON_VERSION:0:3}m 
	else
		PYTHON_M_VERSION=${PYTHON_VERSION:0:3}
	fi	
  
	export PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python 
	export PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION} 
	
	if [[ $(uname) == "Darwin" ]]; then
		export PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.dylib
	else
		
		export PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
	fi
}

# //////////////////////////////////////////////////////
function InstallCMake {

	# already exists?
	if [ -x "$(command -v cmake)" ] ; then
		CMAKE_VERSION=$(cmake --version | cut -d' ' -f3)
		CMAKE_VERSION=${CMAKE_VERSION:0:1}
		if (( CMAKE_VERSION >=3 )); then
			return
		fi	
	fi
	
	if ! [ -x "$BUILD_DIR/cmake/bin/cmake" ]; then
		echo "Downloading precompiled cmake"
		DownloadFile "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
		tar xvzf cmake-3.4.3-Linux-x86_64.tar.gz
		mv cmake-3.4.3-Linux-x86_64 $BUILD_DIR/cmake
	fi
	
	export PATH=$BUILD_DIR/cmake/bin:${PATH} 
}


# //////////////////////////////////////////////////////
function InstallPatchElf {

	# already exists?
	if [ -x "$(command -v patchelf)" ]; then
		return
	fi
	
	# not already compiled?
	if [ ! -f $BUILD_DIR/patchelf/bin/patchelf ]; then
		echo "Compiling patchelf"
		DownloadFile https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz 
		tar xvzf patchelf-0.9.tar.gz
		pushd patchelf-0.9
		./configure --prefix=$BUILD_DIR/patchelf && make && make install
		autoreconf -f -i
		./configure --prefix=$BUILD_DIR/patchelf && make && make install
		popd
	fi
	
	export PATH=$BUILD_DIR/patchelf/bin:$PATH
}


# //////////////////////////////////////////////////////
function InstallBrew {

	if ! [ -x "$(command -v brew)" ]; then
		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	fi
	
	# output is very long!
	brew update 1>/dev/null 2>&1 || true
}



# //////////////////////////////////////////////////////
# NOTE for linux: mixing python openssl and OpenVisus internal openssl cause crashes so I'm always using this one
function InstallOpenSSL {

	if [ ! -f $BUILD_DIR/openssl/lib/libssl.a ]; then
		echo "Compiling openssl"
		DownloadFile "https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
		tar xvzf openssl-1.0.2a.tar.gz 
		pushd openssl-1.0.2a 
		./config -fpic shared 
		make 
		make install	
		popd
	fi
	
	export OPENSSL_ROOT_DIR=/usr/local/ssl
	export OPENSSL_INCLUDE_DIR=${OPENSSL_ROOT_DIR}/include 
	export OPENSSL_LIB_DIR=${OPENSSL_ROOT_DIR}/lib
	export LD_LIBRARY_PATH=${OPENSSL_LIB_DIR}:$LD_LIBRARY_PATH
}


# //////////////////////////////////////////////////////
function InstallSwig {
	if [ ! -f ${BUILD_DIR}/swig/bin/swig ]; then
		DownloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"  
		tar xvzf swig-3.0.12.tar.gz 
		pushd swig-3.0.12 
		DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
		./Tools/pcre-build.sh 
		./configure 
		make -j 4 
		make install 
		popd
	fi 
	export PATH=$BUILD_DIR/swig/bin:${PATH} 
}

# //////////////////////////////////////////////////////
function DetectUbuntuVersion {
	if [ -f /etc/os-release ]; then
		source /etc/os-release
		export OS_VERSION=$VERSION_ID
	elif type lsb_release >/dev/null 2>&1; then
		export OS_VERSION=$(lsb_release -sr)
	elif [ -f /etc/lsb-release ]; then
		source /etc/lsb-release
		export OS_VERSION=$DISTRIB_RELEASE
	fi
	echo "OS_VERSION ${OS_VERSION}"
}



# //////////////////////////////////////////////////////
function InstallApache24 {

	DownloadFile http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz
	tar -xvzf apr-1.6.5.tar.gz
	pushd apr-1.6.5
	./configure && make && make install
	popd
	
	DownloadFile http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz
	tar -xvzf apr-util-1.6.1.tar.gz
	pushd apr-util-1.6.1
	./configure --with-apr=/usr/local/apr && make && make install
	popd
	
	DownloadFile https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz
	tar -xvzf pcre-8.42.tar.gz
	pushd pcre-8.42
	./configure --prefix=/usr/local/pcre && make && make install
	popd
	
	DownloadFile http://it.apache.contactlab.it/httpd/httpd-2.4.37.tar.gz
	tar -xvzf httpd-2.4.37.tar.gz
	pushd httpd-2.4.37
	./configure --with-apr=/usr/local/apr/ --with-pcre=/usr/local/pcre && make && make install
	popd

}

