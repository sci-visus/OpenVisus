#!/bin/bash

set -ex

SOURCE_DIR=$(pwd)

BUILD_DIR=${BUILD_DIR:-$PWD/build}

# cmake flags
PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-1}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

# conda stuff
USE_CONDA=${USE_CONDA:-0}
DEPLOY_CONDA=${DEPLOY_CONDA:-0}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}

# pypi stuff
DEPLOY_PYPI=${DEPLOY_PYPI:-0}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_PASSWORD=${PYPI_PASSWORD:-}

# in case you want to speed up compilation because prerequisites have already been installed
FAST_MODE=${FAST_MODE:-0}

if [ $(uname) = "Darwin" ]; then
  echo "Detected OSX"
  export OSX=1
fi


# //////////////////////////////////////////////////////
function DownloadFile {
  curl -fsSL --insecure "$1" -O
}

# //////////////////////////////////////////////////////
cmake_opts=""
function PushCMakeOption {
  if [ -n "$2" ] ; then
    cmake_opts="${cmake_opts} -D$1=$2"
  fi
}

# //////////////////////////////////////////////////////
function BuildDockerOpenVisus {
  sudo docker rm -f mydocker 2>/dev/null || true

  sudo docker run -d -ti \
    --name mydocker \
    -v ${SOURCE_DIR}:${SOURCE_DIR} \
    -e BUILD_DIR=${BUILD_DIR} \
    -e PYTHON_VERSION=${PYTHON_VERSION} \
    -e VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
    -e DISABLE_OPENMP=${DISABLE_OPENMP} \
    -e VISUS_GUI=${VISUS_GUI} \
    -e CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    -e USE_CONDA=${USE_CONDA} \
    -e DEPLOY_CONDA=${DEPLOY_CONDA} \
    -e ANACONDA_TOKEN=${ANACONDA_TOKEN} \
    -e DEPLOY_PYPI=${DEPLOY_PYPI} \
    -e PYPI_USERNAME=${PYPI_USERNAME} \
    -e PYPI_PASSWORD=${PYPI_PASSWORD} \
    ${DOCKER_IMAGE} \
    /bin/bash

  sudo docker exec mydocker /bin/bash -c "cd ${SOURCE_DIR} && ./build.sh"

  sudo chown -R "$USER":"$USER" ${BUILD_DIR}
  sudo chmod -R u+rwx           ${BUILD_DIR}
}



# //////////////////////////////////////////////////////
function InstallConda {

  if (( OSX ==  1 )) ; then
    if [ ! -d /opt/MacOSX10.9.sdk ] ; then
      git clone https://github.com/phracker/MacOSX-SDKs
      mkdir -p /opt
      sudo mv MacOSX-SDKs/MacOSX10.9.sdk /opt/
      rm -Rf MacOSX-SDKs
    fi
  fi

  if [ ! -d $HOME/miniconda${PYTHON_VERSION:0:1} ]; then
    pushd $HOME
    if (( OSX == 1 )) ; then
      DownloadFile https://repo.continuum.io/miniconda/Miniconda${PYTHON_VERSION:0:1}-latest-MacOSX-x86_64.sh
      bash Miniconda${PYTHON_VERSION:0:1}-latest-MacOSX-x86_64.sh -b
    else
      DownloadFile https://repo.continuum.io/miniconda/Miniconda${PYTHON_VERSION:0:1}-latest-Linux-x86_64.sh
      bash Miniconda${PYTHON_VERSION:0:1}-latest-Linux-x86_64.sh -b
    fi
    popd
  fi

  export PATH="$HOME/miniconda${PYTHON_VERSION:0:1}/bin:$PATH"
  hash -r

  if (( FASTMODE == 0 )) ; then
    conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
    conda install -q conda-build anaconda-client
    conda update  -q conda conda-build
    conda install -q python=${PYTHON_VERSION}
  fi
}

# //////////////////////////////////////////////////////
function InstallOSXPrerequisites {

  if (( FAST_MODE==0 )) ; then

    # install brew
    if [ !  -x "$(command -v brew)" ]; then
      /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
    fi

    # output is very long!
    brew update        1>/dev/null 2>&1 || true
    brew upgrade pyenv 1>/dev/null 2>&1 || true
    brew install swig

    if (( VISUS_INTERNAL_DEFAULT == 0 )); then
      brew install zlib lz4 tinyxml freeimage openssl curl
    fi

    # install qt 5.11 (instead of 5.12 which is not supported by PyQt5)
    # brew install qt5
    if (( VISUS_GUI == 1 )); then
      brew unlink git || true
      brew install https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb
    fi
  fi

  if (( VISUS_GUI == 1 )); then
    PushCMakeOption Qt5_DIR $(brew --prefix Qt)/lib/cmake/Qt5
  fi

}

# //////////////////////////////////////////////////////
function InstallUbuntuPrerequisites {

  # get ubuntu version
  if [ -f /etc/os-release ]; then
    source /etc/os-release
    export UBUNTU_VERSION=$VERSION_ID

  elif type lsb_release >/dev/null 2>&1; then
    export UBUNTU_VERSION=$(lsb_release -sr)

  elif [ -f /etc/lsb-release ]; then
    source /etc/lsb-release
    export UBUNTU_VERSION=$DISTRIB_RELEASE
  fi

  echo "UBUNTU_VERSION ${UBUNTU_VERSION}"

  # install prerequisites
  if (( FAST_MODE==0 )) ; then

    # make sure sudo is available
    if [ "$EUID" -eq 0 ]; then
      apt-get -qq update
      apt-get -qq install sudo
    fi

    sudo apt-get -qq update
    sudo apt-get -qq install git software-properties-common

    if (( ${UBUNTU_VERSION:0:2}<=14 )); then
      sudo add-apt-repository -y ppa:deadsnakes/ppa
      sudo apt-get -qq update
    fi

    sudo apt-get -qq install --allow-unauthenticated \
      cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev \
      uuid-dev curl automake libffi-dev  apache2 apache2-dev

    # install cmake
    echo "Downloading precompiled cmake"
    DownloadFile "https://github.com/Kitware/CMake/releases/download/v3.14.1/cmake-3.14.1-Linux-x86_64.tar.gz"
    tar xvzf cmake-3.14.1-Linux-x86_64.tar.gz -C ${CACHED_DIR} --strip-components=1

    # install patchelf
    DownloadFile https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz
    tar xvzf patchelf-0.9.tar.gz
    pushd patchelf-0.9
    ./configure --prefix=${CACHED_DIR} && make -s && make install
    autoreconf -f -i
    ./configure --prefix=${CACHED_DIR} && make -s && make install
    popd

    if (( VISUS_INTERNAL_DEFAULT == 0 )); then
      sudo apt-get -qq install --allow-unauthenticated zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
    fi

    # install qt (it's a version available on PyQt5)
    if (( VISUS_GUI == 1 )); then

      # https://launchpad.net/~beineri
      # PyQt5 versions 5.6, 5.7, 5.7.1, 5.8, 5.8.1.1, 5.8.2, 5.9, 5.9.1, 5.9.2, 5.10, 5.10.1, 5.11.2, 5.11.3
      if (( ${UBUNTU_VERSION:0:2} <=14 )); then
        sudo add-apt-repository ppa:beineri/opt-qt-5.10.1-trusty -y
        sudo apt-get -qq update
        sudo apt-get install -yqq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt510base

      elif (( ${UBUNTU_VERSION:0:2} <=16 )); then
        sudo add-apt-repository ppa:beineri/opt-qt-5.11.2-xenial -y
        sudo apt-get -qq update
        sudo apt-get install -yqq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt511base

      elif (( ${UBUNTU_VERSION:0:2} <=18)); then
        sudo add-apt-repository ppa:beineri/opt-qt-5.11.2-bionic -y
        sudo apt-get -qq update
        sudo apt-get install -yqq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt511base
      fi

    fi

  fi

  if (( ${UBUNTU_VERSION:0:2} <=14 )); then
    PushCMakeOption Qt5_DIR /opt/qt510/lib/cmake/Qt5

  elif (( ${UBUNTU_VERSION:0:2} <=16 )); then
    PushCMakeOption Qt5_DIR /opt/qt511/lib/cmake/Qt5

  elif (( ${UBUNTU_VERSION:0:2} <=18)); then
    PushCMakeOption Qt5_DIR /opt/qt511/lib/cmake/Qt5

  fi

  PushCMakeOption SWIG_EXECUTABLE $(which swig3.0)
}

# //////////////////////////////////////////////////////
function InstallOpenSusePrerequisites {

  # install prerequisites
  if (( FAST_MODE==0 )) ; then

    # make sure sudo is available
    if [ "$EUID" -eq 0 ]; then
      zypper --non-interactive update
      zypper --non-interactive install sudo
    fi

    sudo zypper --non-interactive update
    sudo zypper --non-interactive install --type pattern devel_basis
    sudo zypper --non-interactive install lsb-release gcc-c++ cmake git swig  libuuid-devel libopenssl-devel curl patchelf apache2 apache2-devel libffi-devel

    if (( VISUS_INTERNAL_DEFAULT == 0 )); then
      sudo zypper --non-interactive install zlib-devel liblz4-devel tinyxml-devel libfreeimage-devel libcurl-devel
    fi

    if (( VISUS_GUI==1 )); then
      sudo zypper -n in  glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel
    fi

  fi
}

# //////////////////////////////////////////////////////
function InstallCentosPrerequisites {

  if (( FAST_MODE==0 )) ; then

    yum update
    yum install -y zlib-devel curl libffi-devel

    # install openssl
    echo "Installing openssl"
    DownloadFile "https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
    tar xzf openssl-1.0.2a.tar.gz
    pushd openssl-1.0.2a
    ./config --prefix=/usr -fpic shared
    make -s
    make install
    popd

    # install cmake
    echo "Downloading precompiled cmake"
    DownloadFile "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
    tar xvzf cmake-3.4.3-Linux-x86_64.tar.gz -C ${CACHED_DIR} --strip-components=1

    # install swig
    echo "Istalling swig"
    DownloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"
    tar xzf swig-3.0.12.tar.gz
    pushd swig-3.0.12
    DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
    ./Tools/pcre-build.sh
    ./configure --prefix=${CACHED_DIR}
    make -s -j 4
    make install
    popd

    # install apache 2.4
    # for centos5 this is 2.2, I prefer to use 2.4 which is more common
    # yum install -y httpd.x86_64 httpd-devel.x86_64
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

    DownloadFile http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz
    tar xzf httpd-2.4.38.tar.gz
    pushd httpd-2.4.38
    ./configure --prefix=${CACHED_DIR} --with-apr=${CACHED_DIR} --with-pcre=${CACHED_DIR} && make -s && make install
    popd

  fi

  PushCMakeOption APACHE_DIR ${CACHED_DIR}
  PushCMakeOption APR_DIR    ${CACHED_DIR}
}

# //////////////////////////////////////////////////////
function InstallPython {

  if ! [ -d "$HOME/.pyenv" ]; then
    pushd $HOME
    DownloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
    chmod a+x pyenv-installer
    ./pyenv-installer
    rm -f pyenv-installer
    popd
  fi

  export PATH="$HOME/.pyenv/bin:$PATH"
  eval "$(pyenv init -)"

  # install python
  if (( FAST_MODE==0 )) ; then

    # pyenv install --list
    __CFLAGS__=""
    __CPPFLAGS__=""
    __LDFLAGS__=""

    # this is necessary for osx 10.14
    if (( OSX == 1 )) ; then
      brew install zlib openssl
      __CFLAGS__+="   -I$(brew --prefix zlib)/include"
      __CPPFLAGS__+=" -I$(brew --prefix zlib)/include"
      __LDFLAGS__+="  -L$(brew --prefix zlib)/lib"
      __CFLAGS__+="   -I$(brew --prefix openssl)/include"
      __CPPFLAGS__+=" -I$(brew --prefix openssl)/include"
      __LDFLAGS__+="  -L$(brew --prefix openssl)/lib"
    else
      export CXX=g++
      if [[ "$OPENSSL_INCLUDE_DIR" != "" ]] ; then
        __CFLAGS__+="-I${OPENSSL_INCLUDE_DIR}"
        __CPPFLAGS__+="-I${OPENSSL_INCLUDE_DIR}"
        __LDFLAGS__+="-L${OPENSSL_LIB_DIR}"
      fi
    fi

    CONFIGURE_OPTS="--enable-shared" CFLAGS="${__CFLAGS__}" CPPFLAGS="${__CPPFLAGS__}" LDFLAGS="${__LDFLAGS__}" pyenv install --skip-existing ${PYTHON_VERSION}

    if (( OSX != 1 )); then
      unset CXX
    fi

    pyenv global ${PYTHON_VERSION}
    pyenv rehash
    python -m pip install --upgrade pip
    python -m pip install --upgrade numpy setuptools wheel twine auditwheel

  fi

  # activate pyenv
  pyenv global ${PYTHON_VERSION}
  pyenv rehash

  if [ "${PYTHON_VERSION:0:1}" -gt "2" ]; then
    PYTHON_M_VERSION=${PYTHON_VERSION:0:3}m
  else
    PYTHON_M_VERSION=${PYTHON_VERSION:0:3}
  fi

  PushCMakeOption PYTHON_EXECUTABLE   $(pyenv prefix)/bin/python
  PushCMakeOption PYTHON_INCLUDE_DIR  $(pyenv prefix)/include/python${PYTHON_M_VERSION}

  if (( OSX == 1 )) ; then
    PushCMakeOption PYTHON_LIBRARY $(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.dylib
  else
    PushCMakeOption PYTHON_LIBRARY $(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
  fi
}

# //////////////////////////////////////////////////////
function BuildCondaOpenVisus {
	pushd conda
	conda-build -q openvisus
	conda install -q --use-local openvisus
	CONDA_BUILD_FILENAME=$(find ${HOME}/miniconda${PYTHON_VERSION:0:1}/conda-bld -iname "openvisus*.tar.bz2")
	popd
}

# //////////////////////////////////////////////////////
function BuildOpenVisus {

	PushCMakeOption PYTHON_VERSION         ${PYTHON_VERSION}
	PushCMakeOption VISUS_INTERNAL_DEFAULT ${VISUS_INTERNAL_DEFAULT}
	PushCMakeOption DISABLE_OPENMP         ${DISABLE_OPENMP}
	PushCMakeOption VISUS_GUI              ${VISUS_GUI}
	PushCMakeOption CMAKE_BUILD_TYPE       ${CMAKE_BUILD_TYPE}

	if (( OSX == 1 )) ; then

	 cmake -GXcode ${cmake_opts} ${SOURCE_DIR}

	 # this is to solve logs too long
	 if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
		sudo gem install xcpretty
		set -o pipefail && cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
	 else
		cmake                    --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE}
	 fi

	 cmake --build ./ --target install --config ${CMAKE_BUILD_TYPE}

	else
		cmake ${cmake_opts} ${SOURCE_DIR}
		cmake --build . --target all -- -j 4
		cmake --build . --target install
	fi


	if (( OSX == 1 )) ; then
	 	cmake --build ./ --target dist    --config ${CMAKE_BUILD_TYPE}
	 	WHEEL_FILENAME=$(find ${BUILD_DIR}/RelWithDebInfo/site-packages/OpenVisus/dist -iname "*.whl")
	else
	 	cmake --build ./ --target dist
	 	WHEEL_FILENAME=$(find ${BUILD_DIR}/site-packages/OpenVisus/dist -iname "*.whl")
	fi

		
	python -m pip install --ignore-installed "${WHEEL_FILENAME}"  	
}

# //////////////////////////////////////////////////////
function TestOpenVisus {

	cd $HOME
	python -m OpenVisus CreateScripts
	cd $(python -m OpenVisus dirname)
	
	python Samples/python/Array.py
	python Samples/python/Dataflow.py
	python Samples/python/Idx.py

	if (( OSX == 1 )) ; then
		if (( USE_CONDA == 1 )) ; then
			echo "Skipping execution of visus.command"
			echo "conda standalone commands are not working"
			echo "reason: dyld: Symbol not found: _iconv	"		
		else
			./visus.command
		fi
	else
		./visus.sh
	fi
}


# //////////////////////////////////////////////////////
function DeployPyPi {
	echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
	echo [distutils]                                  > ~/.pypirc
	echo index-servers =  pypi                       >> ~/.pypirc
	echo [pypi]                                      >> ~/.pypirc
	echo username=${PYPI_USERNAME}                   >> ~/.pypirc
	echo password=${PYPI_PASSWORD}                   >> ~/.pypirc
	python -m twine upload --skip-existing "${WHEEL_FILENAME}"
}

# //////////////////////////////////////////////////////
function DeployConda {
	echo "Doing deploy to anaconda ${CONDA_BUILD_FILENAME}..."
	anaconda -q -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
}


if [[ "$DOCKER_IMAGE" != "" ]] ; then

  BuildDockerOpenVisus
  exit 0

elif (( USE_CONDA == 1 )) ; then

  InstallConda
  BuildCondaOpenVisus
  TestOpenVisus

  if (( DEPLOY_CONDA == 1 )) ; then
    DeployConda
  fi

else

	# directory for caching install stuff
	CACHED_DIR=${BUILD_DIR}/cached_deps
	mkdir -p ${CACHED_DIR}

	export PATH=${CACHED_DIR}/bin:$PATH
	mkdir -p ${BUILD_DIR}

  cd ${BUILD_DIR}

	if (( OSX == 1 )) ; then
    echo "Detected OSX"
    InstallOSXPrerequisites

	elif [ -x "$(command -v apt-get)" ]; then
		echo "Detected ubuntu"
    InstallUbuntuPrerequisites

	elif [ -x "$(command -v zypper)" ]; then
		echo "Detected opensuse"
    InstallOpenSusePrerequisites

	elif [ -x "$(command -v yum)" ]; then
		echo "Detected centos"
    InstallCentosPrerequisites

	else
		echo "Failed to detect OS version, I will keep going but it could be that I won't find some dependency"

	fi

  InstallPython
  BuildOpenVisus
  TestOpenVisus

  if (( DEPLOY_PYPI == 1 )) ; then
    DeployPyPi
  fi

fi

echo "OpenVisus build finished"
set +ex
