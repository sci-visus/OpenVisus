#!/bin/bash

set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
VISUS_GUI=${VISUS_GUI:-1} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release} 
BUILD_DIR=${BUILD_DIR:-$(pwd)/build/opensuse} 

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# //////////////////////////////////////////////////////
function DownloadFile {
	curl -fsSL --insecure "$1" -O
}

# //////////////////////////////////////////////////////
function PushCMakeOption {
	if [ -n "$2" ] ; then
		cmake_opts+=" -D$1=$2"
	fi
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
	export PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
}

# //////////////////////////////////////////////////////
function InstallQt {
  sudo zypper -n in  glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel 
}

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

cmake_opts=""
PushCMakeOption PYTHON_VERSION         ${PYTHON_VERSION}
PushCMakeOption CMAKE_BUILD_TYPE       ${CMAKE_BUILD_TYPE}
PushCMakeOption VISUS_INTERNAL_DEFAULT ${VISUS_INTERNAL_DEFAULT}
PushCMakeOption VISUS_GUI              ${VISUS_GUI}
PushCMakeOption PYTHON_EXECUTABLE      ${PYTHON_EXECUTABLE}
PushCMakeOption PYTHON_INCLUDE_DIR     ${PYTHON_INCLUDE_DIR}
PushCMakeOption PYTHON_LIBRARY         ${PYTHON_LIBRARY}

cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 
cmake --build . --target deploy 
cmake --build . --target bdist_wheel
cmake --build . --target sdist 

cd install
LD_LIBRARY_PATH=$(pwd):$(dirname ${PYTHON_LIBRARY}) PYTHONPATH=$(pwd) bin/visus     && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd) PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import OpenVisus" && echo "Extending working"






