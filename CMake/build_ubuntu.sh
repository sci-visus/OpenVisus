#!/bin/bash

set -ex 

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6} 
VISUS_GUI=${VISUS_GUI:-1} 
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-0} 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}
BUILD_DIR=${BUILD_DIR:-$(pwd)/build/ubuntu} 

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# detect version
if [ -f /etc/os-release ]; then
	source /etc/os-release
	OS_VERSION=$VERSION_ID
elif type lsb_release >/dev/null 2>&1; then
	OS_VERSION=$(lsb_release -sr)
elif [ -f /etc/lsb-release ]; then
	source /etc/lsb-release
	OS_VERSION=$DISTRIB_RELEASE
fi
echo "OS_VERSION ${OS_VERSION}"

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

	# i can also do this
	if ((0)); then
		sudo apt-get install -y software-properties-common 
		sudo add-apt-repository ppa:jonathonf/python-${PYTHON_VERSION:0:3}
		sudo apt-get update
		sudo apt-get install -y python${PYTHON_VERSION:0:3} python${PYTHON_VERSION:0:3}-dev
		downloadFile https://bootstrap.pypa.io/get-pip.py /
		export PYTHON_EXECUTABLE=$(which python3.6) 
		sudo ${PYTHON_EXECUTABLE} get-pip.py
		sudo ${PYTHON_EXECUTABLE} -m pip install --upgrade numpy setuptools wheel twine auditwheel  
		return
	fi

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
	python -m pip install --upgrade numpy setuptools wheel twine auditwheel 

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
function InstallQt {
	if (( ${OS_VERSION:0:2} <=14 )); then
		sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y
		sudo apt-get update -qq
		sudo apt-get install -qq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt59base
		set +e # temporary disable exit
		source /opt/qt59/bin/qt59-env.sh 
		set -e 
	else	
		sudo apt-get install -qq qt5-default qttools5-dev-tools	
	fi
}

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

InstallCMake
InstallPatchElf 
InstallPython 

if (( VISUS_INTERNAL_DEFAULT==0 )); then 
  sudo apt-get -qq install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

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
PushCMakeOption SWIG_EXECUTABLE        $(which swig3.0)

# compile OpenVisus
cmake ${cmake_opts} ${SOURCE_DIR} 
 
cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 
cmake --build . --target deploy 

pushd install
LD_LIBRARY_PATH=$(pwd):$(dirname ${PYTHON_LIBRARY}) PYTHONPATH=$(pwd) bin/visus     && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd) PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import OpenVisus"	&& echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi

if (( DEPLOY_PYPI == 1 )); then
	cmake --build ./ --target bdist_wheel --config ${CMAKE_BUILD_TYPE} 
	cmake --build ./ --target pypi        --config ${CMAKE_BUILD_TYPE}
fi




