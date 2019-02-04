#!/bin/bash


# //////////////////////////////////////////////////////
function DetectOS {
	
	if [ -n "${DOCKER_IMAGE}" ]; then
		./CMake/build_docker.sh

	elif [ $(uname) = "Darwin" ]; then
		echo "Detected OSX"
		export OSX=1

	elif [ -x "$(command -v apt-get)" ]; then
		echo "Detected ubuntu"
		export UBUNTU=1
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

	elif [ -x "$(command -v zypper)" ]; then
		echo "Detected opensus"
		export OPENSUSE=1
		
	elif [ -x "$(command -v yum)" ]; then
		echo "Detected manylinux"
		export MANYLINUX

	else
		echo "Failed to detect OS version"
	fi
}

DetectOS

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

}

# //////////////////////////////////////////////////////
function InstallPatchElf {

	# already exists?
	if [ -x "$(command -v patchelf)" ]; then
		return
	fi
	
	echo "Compiling patchelf"
	DownloadFile https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz 
	tar xvzf patchelf-0.9.tar.gz
	pushd patchelf-0.9
	./configure --prefix=${CACHED_DIR} && make -s && make install
	autoreconf -f -i
	./configure --prefix=${CACHED_DIR} && make -s && make install
	popd
}


# //////////////////////////////////////////////////////
function InstallOpenSSL {

   # NOTE this requires root, I'm doing this because using a custom directory does not work in python
	#if [ ! -f ${CACHED_DIR}/openssl-1.0.2a.done ]; then
		echo "Compiling openssl"
		DownloadFile "https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
		tar xzf openssl-1.0.2a.tar.gz 
		pushd openssl-1.0.2a 
		./config --prefix=/usr -fpic shared 
		make -s 
		make install	
		touch ${CACHED_DIR}/openssl-1.0.2a.done
		popd
	#fi
}


# //////////////////////////////////////////////////////
function InstallSwig {

	if [ -x "${CACHED_DIR}/bin/swig" ]; then
		return
	fi
	
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

	if ! [ -x "${CACHED_DIR}/bin/cmake" ]; then
		echo "Downloading precompiled cmake"
		DownloadFile "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
		tar xvzf cmake-3.4.3-Linux-x86_64.tar.gz -C ${CACHED_DIR} --strip-components=1 
	fi
}

# //////////////////////////////////////////////////////
function InstallBrew {

	if ! [ -x "$(command -v brew)" ]; then
		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	fi
	
	# output is very long!
	brew update        1>/dev/null 2>&1 || true
	brew upgrade pyenv 1>/dev/null 2>&1 || true
}

# //////////////////////////////////////////////////////
function InstallMiniconda {
	
	if [ -x "$(command -v conda)" ]; then
		return
	fi
	
	pushd $HOME
	wget -q https://repo.continuum.io/miniconda/Miniconda${PYTHON_VERSION:0:1}-latest-Linux-x86_64.sh -O miniconda.sh
	bash miniconda.sh -b 
	export PATH="$HOME/miniconda${PYTHON_VERSION:0:1}/bin:$PATH"
	hash -r
	conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
	conda install -q conda-build anaconda-client 
	conda update  -q conda conda-build
	conda install -q python=${PYTHON_VERSION} 
	popd	
}

# //////////////////////////////////////////////////////
function InstallQt5ForUbuntu {

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
}

# //////////////////////////////////////////////////////
function ActivatePyEnvPython {
	
	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"
	
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
	
	if (( $(uname) = "Darwin" )) ; then
		export PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.dylib
	else
		
		export PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
	fi
}

# //////////////////////////////////////////////////////
function InstallPyEnvPython {

   	# need to install pyenv?
	if ! [ -d "$HOME/.pyenv" ]; then
		DownloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
		chmod a+x pyenv-installer 
		./pyenv-installer 
		rm -f pyenv-installer 
	fi
	
	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"
	eval "$(pyenv virtualenv-init -)"
	
	# pyenv install --list
	__CFLAGS__=""
	__CPPFLAGS__=""
	__LDFLAGS__=""
	
	# tnis is necessary for osx 10.14
	if (( $(uname) = "Darwin" )) ; then
		brew install zlib openssl
		__CFLAGS__+="-I/usr/local/opt/zlib/include    -I/usr/local/opt/openssl/include"
		__CPPFLAGS__+="-I/usr/local/opt/zlib/include  -I/usr/local/opt/openssl/include"
		__LDFLAGS__+="-L/usr/local/opt/zlib/lib       -L/usr/local/opt/openssl/lib"
	else
		export CXX=g++
		if [ -n "${OPENSSL_INCLUDE_DIR}" ]; then
			__CFLAGS__+="-I${OPENSSL_INCLUDE_DIR}"
			__CPPFLAGS__+="-I${OPENSSL_INCLUDE_DIR}"
			__LDFLAGS__+="-L${OPENSSL_LIB_DIR}"
		fi		
		
	fi
	
	CONFIGURE_OPTS="--enable-shared" CFLAGS="${__CFLAGS__}" CPPFLAGS="${__CPPFLAGS__}" LDFLAGS="${__LDFLAGS__}" pyenv install --skip-existing ${PYTHON_VERSION}  

	if [[ "$(uname)" != "Darwin"   ]]; then
		unset CXX
	fi
	
	ActivatePyEnvPython
}

# //////////////////////////////////////////////////////
function DeployPypi {
	
	echo [distutils]                                  > ~/.pypirc
	echo index-servers =  pypi                       >> ~/.pypirc
	echo [pypi]                                      >> ~/.pypirc
	echo username=${PYPI_USERNAME}                   >> ~/.pypirc
	echo password=${PYPI_PASSWORD}                   >> ~/.pypirc
	python -m twine upload --skip-existing "$1" 	
	
}







