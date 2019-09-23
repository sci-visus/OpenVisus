#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

# forward to Docker (rehentrant)
if [[ "${DOCKER_IMAGE}" != "" ]] ; then
	./CMake/build_docker.sh
	exit 0
fi

# forward to conda (non rehentrant)
if [[ "${USE_CONDA}" == "1" ]] ; then 
	./CMake/build_conda.sh
	exit 0
fi

# forward to osx (non rehentrant)
if [ "$(uname)" == "Darwin" ]; then
	./CMake/build_osx.sh
	exit 0
fi

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}
CACHE_DIR=${CACHE_DIR:-${BUILD_DIR}/.cache}

# you can enable/disable certain options 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
VISUS_NET=${VISUS_NET:-1}
VISUS_IMAGE=${VISUS_IMAGE:-1}
VISUS_PYTHON=${VISUS_PYTHON:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
VISUS_GUI=${VISUS_GUI:-1}

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6}

# //////////////////////////////////////////////////////
# return the next word after the pattern and parse the version in the format MAJOR.MINOR.whatever
function GetVersionFromCommand {	

	set +e

	__content__=$(${1} 2>/dev/null)
	__version__=$(echo ${__content__} | awk -F "${2}" '{print $2}' | cut -d' ' -f1)

	__major__=0
	__minor__=0 
	__patch__=0 

	__major__=$(echo ${__version__} | cut -d'.' -f1)
	__minor__=$(echo ${__version__} | cut -d'.' -f2) 
	__patch__=$(echo ${__version__} | cut -d'.' -f3) 

	__major__=$(expr ${__major__}     + 0)
	__minor__=$(expr ${__minor__}     + 0)
	__patch__=$(expr ${__patch__:0:1} + 0) # only first character

	if (( __major__ >0 &&  __minor__ >= 0 && __patch__ >= 0 )); then
		echo "Found version(${__version__}) major(${__major__}) minor(${__minor__}) patch(${__patch__}) "
	else
		echo "Wrong version(${__version__}) major(${__major__}) minor(${__minor__}) patch(${__patch__}) "
		__version__=""
		__major__=0
		__minor__=0
		__patch__=0
	fi

	set -e
	return 0
}


# ///////////////////////////////////////////////////////////////////////////////////////////////
function BeginSection {
	set +x	
	echo "//////////////////////////////////////////////////////////////////////// $1"
	set -x
}


# //////////////////////////////////////////////////////
function IsPackageInstalled {

	if (( UBUNTU == 1 )) ; then
		dpkg -s $1  1>/dev/null 2>/dev/null && : 
		if [ $? == 0 ] ; then return 0 ; fi

	elif (( OPENSUSE == 1 )) ; then
		rpm -q $1 1>/dev/null 2>/dev/null && : 
		if [ $? == 0 ] ; then return 0 ; fi

	elif (( CENTOS == 1 )) ; then
		yum --quiet -y list installed $1 1>/dev/null 2>/dev/null && : 
		if [ $? == 0 ] ; then return 0 ; fi
	fi

	return -1
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function NeedPatchElf {

	if [ -x "$(command -v patchelf)" ]; then
		return 0
	fi

	if [ ! -f "${CACHE_DIR}/bin/patchelf" ] ; then
		curl -fsSL --insecure "https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz" | tar xz			
		pushd patchelf-0.9 
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null 
		autoreconf -f -i  
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s 1>/dev/null 
		make install 1>/dev/null
		popd 
	fi
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function NeedCMake {

	if [[ -x "$(command -v cmake)" ]]; then
		GetVersionFromCommand "cmake --version" "cmake version "
		if (( __major__== 3 && __minor__ >= 9 )); then
			echo "Good version: cmake==${__version__}"
			return 0
		fi
	fi

	if [ ! -f "${CACHE_DIR}/bin/cmake" ]; then
		echo "installing cached cmake"
		CMAKE_VERSION=3.10.1
		if (( CENTOS == 1 && CENTOS_MAJOR <= 5 )) ; then  
			CMAKE_VERSION=3.4.3  # Error with other  versions: `GLIBC_2.6' not found (required by cmake)
		fi 
		url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz"
		curl -fsSL --insecure ${url}  | tar xz -C ${CACHE_DIR} --strip-components=1 
	fi
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function NeedSwig {

	if [[ "${SWIG_EXECUTABLE}" != "" ]] ; then 
		return 0
	fi

	if [[ -x "$(command -v swig3.0)" ]]; then
		return 0
	fi

	if [[ -x "$(command -v swig)" ]]; then
		GetVersionFromCommand "swig -version" "SWIG Version "
		if (( __major__>= 3)); then
			return 0
		fi
	fi 

	export SWIG_EXECUTABLE=${CACHE_DIR}/bin/swig
	if [ ! -f "${SWIG_EXECUTABLE}" ]; then
		BeginSection "Compiling swig"
		curl -fsSL --insecure "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz" | tar xz
		pushd swig-3.0.12 
		curl -fsSL --insecure  "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" -O
		./Tools/pcre-build.sh 1>/dev/null
		./configure --prefix=${CACHE_DIR} 1>/dev/null 
		make -s -j 4 1>/dev/null 
		make install 1>/dev/null 
		popd
	fi
}




# ///////////////////////////////////////////////////////////////////////////////////////////////
function NeedPython {

	BeginSection "Installing python"

	# install pyenv
	if [ ! -f "$HOME/.pyenv/bin/pyenv" ]; then
		pushd $HOME
		curl -fsSL --insecure  "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer" -O
		chmod a+x pyenv-installer
		./pyenv-installer
		rm -f pyenv-installer
		popd
	fi

	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"

	# try to use the existing openssl
	__good_openssl__=0
	if [ -x "/usr/bin/openssl" ] ; then

		GetVersionFromCommand "/usr/bin/openssl version" "OpenSSL "

		# Python requires an OpenSSL 1.0.2 or 1.1 compatible libssl with X509_VERIFY_PARAM_set1_host()
		if (( __major__ > 1 || ( __major__ == 1 && __minor__ >= 1 ) || ( __major__ == 1 && __minor__ == 0 && __patch__ >= 2 ) )) ; then
			echo "Openssl version(${__version__}) ok"
			__good_openssl__=1
		else
			echo "OpenSSL version(${__version__}) wrong"
		fi
	fi

	if (( __good_openssl__ == 0 )) ; then
		export LD_LIBRARY_PATH="${CACHE_DIR}/lib:${LD_LIBRARY_PATH}"
		if [ ! -f "${CACHE_DIR}/bin/openssl" ]; then
			curl -fsSL --insecure  "https://www.openssl.org/source/openssl-1.0.2a.tar.gz" | tar xz
			pushd openssl-1.0.2a 
			./config --prefix=${CACHE_DIR} -fPIC shared 1>/dev/null  
			make -s      1>/dev/null  
			make install 1>/dev/null 
			popd
		fi

		export CONFIGURE_OPTS="--with-openssl=${CACHE_DIR}"
		export CFLAGS="  -I${CACHE_DIR}/include -I${CACHE_DIR}/include/openssl"
		export CPPFLAGS="-I${CACHE_DIR}/include -I${CACHE_DIR}/include/openssl"
		export LDFLAGS=" -L${CACHE_DIR}/lib" 
	fi

	CXX=g++ CONFIGURE_OPTS="${CONFIGURE_OPTS} --enable-shared" pyenv install --skip-existing ${PYTHON_VERSION}
	unset CFLAGS
	unset CPPFLAGS
	unset LDFLAGS

	# activate pyenv
	export PATH="$HOME/.pyenv/bin:$PATH"
	eval "$(pyenv init -)"
	pyenv global ${PYTHON_VERSION}
	pyenv rehash

	PYTHON_M_VERSION=${PYTHON_VERSION:0:1}.${PYTHON_VERSION:2:1}
	if (( ${PYTHON_VERSION:0:1} > 2 )) ; then PYTHON_M_VERSION=${PYTHON_M_VERSION}m ; fi

	PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
	PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION}
	PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so

	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine auditwheel	
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function NeedApache {

	# use existing
	if (( MANYLINUX == 0 )); then
		unset APACHE_DIR
		return 0
	fi

	APACHE_DIR=${CACHE_DIR}
	if [ ! -f "${APACHE_DIR}/include/httpd.h" ] ; then

		BeginSection "Compiling apache from source"

		curl -fsSL --insecure "https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-2.2.6.tar.bz2" | tar xj
		pushd expat-2.2.6 
		./configure --prefix=${APACHE_DIR} 1>/dev/null
		make -s      1>/dev/null  
		make install 1>/dev/null 
		popd

		curl -fsSL --insecure "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" | tar xz
		pushd pcre-8.42 
		./configure --prefix=${APACHE_DIR} 1>/dev/null 
		make -s      1>/dev/null  
		make install 1>/dev/null 
		popd

		curl -fsSL --insecure "http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz" | tar xz 
		pushd httpd-2.4.38		
		curl -fsSL --insecure "http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz"      | tar xz
		curl -fsSL --insecure "http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz" | tar xz	
		mv ./apr-1.6.5      ./srclib/apr
		mv ./apr-util-1.6.1 ./srclib/apr-util
		./configure --prefix=${APACHE_DIR} --with-included-apr --with-pcre=${APACHE_DIR} --with-ssl=${APACHE_DIR} --with-expat=${APACHE_DIR}  1>/dev/null 
		make -s       1>/dev/null 
		make install  1>/dev/null 
		popd

	fi	
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function NeedQt5 
{
	if [[ "${Qt5_DIR}" != "" ]] ; then	
		return 0
	fi

	# try to use existing Qt5
	if (( UBUNTU == 1 )) ; then

		if [[ -d  "/opt/qt510/lib/cmake/Qt5" ]]; then
			Qt5_DIR="/opt/qt510/lib/cmake/Qt5"
			return 0
		fi

		if [[ -d  "/opt/qt511/lib/cmake/Qt5" ]]; then
			Qt5_DIR="/opt/qt511/lib/cmake/Qt5"
			return 0
		fi

	fi

	# backup plan , use a minimal Qt5 which does not need SUDO
	# if you want to create a "new" minimal Qt5 see CMake/Dockerfile.BuildQt5
	# note this is only to allow compilation
	# in order to execute it you need to use PyUseQt 
	echo "Using minimal Qt5"
	Qt5_DIR=${CACHE_DIR}/qt5.11.2/lib/cmake/Qt5
	if [ ! -d "${Qt5_DIR}" ] ; then
		curl -fsSL --insecure  "http://atlantis.sci.utah.edu/qt/qt5.11.2.tar.gz" | tar xz -C ${CACHE_DIR} 
	fi
}

# /////////////////////////////////////////////////////////////////////
function AddCMakeOption {
	key=$1
	value=$2
	if [[ "${value}" != "" ]]; then
		cmake_opts+=(${key}=${value})
	fi
}

if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
	export TRAVIS=1 
fi

IsRoot=0
if (( EUID== 0 || TRAVIS == 1 )); then 
	IsRoot=1
else
	grep 'docker\|lxc' /proc/1/cgroup && :
	if [ $? == 0 ] ; then  IsRoot=1 ; fi
fi

mkdir -p ${BUILD_DIR}
mkdir -p ${CACHE_DIR}
export PATH=${CACHE_DIR}/bin:$PATH

# compiler and essential packages
if (( IsRoot == 1 )) ; then

	packages=""

	# install essential packages
	if [ -x "$(command -v apt-get)" ]; then
		UBUNTU=1

		if [ -f /etc/os-release ]; then
			source /etc/os-release
			export UBUNTU_VERSION=$VERSION_ID

		elif type lsb_release >/dev/null 2>&1; then
			export UBUNTU_VERSION=$(lsb_release -sr)

		elif [ -f /etc/lsb-release ]; then
			source /etc/lsb-release
			export UBUNTU_VERSION=$DISTRIB_RELEASE
		fi

		OS_NICKNAME=ubuntu.${UBUNTU_VERSION}
		echo "Detected ubuntu ${UBUNTU_VERSION}"
			
		apt-get --quiet --yes --allow-unauthenticated update 
		apt-get --quiet --yes --allow-unauthenticated install software-properties-common 
		if (( ${UBUNTU_VERSION:0:2}<=14 )); then
			add-apt-repository -y ppa:deadsnakes/ppa
			apt-get --quiet --yes --allow-unauthenticated update
		fi

		packages+=" build-essential make automake git curl ca-certificates"
		packages+=" uuid-dev bzip2 libffi-dev libssl-dev swig3.0 swig swig3.0 swig cmake patchelf"

		if (( VISUS_MODVISUS == 1 )); then
			packages+=" apache2 apache2-dev"
		fi

		if (( VISUS_GUI == 1 )) ; then

			packages+=" mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev "

			# https://launchpad.net/~beineri
			# PyQt5 versions 5.6, 5.7, 5.7.1, 5.8, 5.8.1.1, 5.8.2, 5.9, 5.9.1, 5.9.2, 5.10, 5.10.1, 5.11.2, 5.11.3
			if (( ${UBUNTU_VERSION:0:2} <=14 )); then
				add-apt-repository  ppa:beineri/opt-qt-5.10.1-trusty -y && :
				packages+=" qt510base"

			elif (( ${UBUNTU_VERSION:0:2} <=16 )); then
				add-apt-repository  ppa:beineri/opt-qt-5.11.2-xenial -y && :
				packages+=" qt511base"

			elif (( ${UBUNTU_VERSION:0:2} <=18)); then
				add-apt-repository  ppa:beineri/opt-qt-5.11.2-bionic -y && :
				packages+=" qt511base"

			else
				echo "internal error"
				exit -1
			fi

			apt-get --quiet --yes --allow-unauthenticated update && :

		fi

		# install one by one otherwise it will fail
		for package in ${packages}
		do
			apt-get --quiet --yes --allow-unauthenticated install ${package}  && :
		done

	elif [ -x "$(command -v zypper)" ]; then
		OPENSUSE=1
		OS_NICKNAME=opensuse
		echo "Detected opensuse"

		zypper --quiet --non-interactive update 
		zypper --quiet --non-interactive install --type pattern devel_basis 

		packages+=" gcc-c++ make  git curl lsb-release libuuid-devel libffi-devel libopenssl-devel swig3.0 swig cmake patchelf"

		if (( VISUS_MODVISUS == 1 )); then
			packages+=" apache2 apache2-devel"
		fi

		if (( VISUS_GUI ==  1 )); then
			packages+=" glu-devel"
		fi

		# install one by one otherwise it will fail
		for package in ${packages} 
		do
			zypper --quiet --non-interactive install ${package}  && :
		done

	elif [ -x "$(command -v yum)" ]; then

		CENTOS=1
		GetVersionFromCommand "cat /etc/redhat-release" "CentOS release "
		CENTOS_VERSION=${__version__}
		CENTOS_MAJOR=${__major__}
		OS_NICKNAME=centos.${CENTOS_VERSION}
		echo "Detected centos ${CENTOS_VERSION}"

		if (( CENTOS_MAJOR == 5 )) ; then
			echo "Detected manylinux"
			MANYLINUX=1
		fi

		yum --quiet -y update 

		packages+=" gcc-c++ make git curl zlib zlib-devel libffi-devel openssl-devel swig3.0 swig cmake patchelf"

		if (( VISUS_GUI ==  1 )); then
			packages+=" mesa-libGL-devel mesa-libGLU-devel" 
		fi

		# install one by one otherwise it will fail
		for package in ${packages}
		do
			yum --quiet -y install ${package}  && :
		done

	else
		echo "Failed to detect OS version, I will keep going but it could be that I won't find some dependency"
	fi

fi

cd ${BUILD_DIR}

NeedPatchElf
NeedCMake

if (( VISUS_PYTHON == 1 )) ; then
	NeedSwig
	NeedPython
fi

if (( VISUS_MODVISUS == 1 )); then
	NeedApache
fi

if (( VISUS_GUI == 1 )); then
	NeedQt5
fi

declare -a cmake_opts

AddCMakeOption -DCMAKE_BUILD_TYPE   "${CMAKE_BUILD_TYPE}"
AddCMakeOption -DVISUS_NET          "${VISUS_NET}"
AddCMakeOption -DVISUS_IMAGE        "${VISUS_IMAGE}"

# python
AddCMakeOption -DVISUS_PYTHON "${VISUS_PYTHON}"
if (( VISUS_PYTHON == 1 )) ; then
	AddCMakeOption -DSWIG_EXECUTABLE      "${SWIG_EXECUTABLE}"
	AddCMakeOption -DPYTHON_VERSION       "${PYTHON_VERSION}"
	AddCMakeOption -DPYTHON_EXECUTABLE    "${PYTHON_EXECUTABLE}"
	AddCMakeOption -DPYTHON_INCLUDE_DIR   "${PYTHON_INCLUDE_DIR}"
	AddCMakeOption -DPYTHON_LIBRARY       "${PYTHON_LIBRARY}"
fi

# mod_visus
if (( VISUS_MODVISUS == 1 )); then
	AddCMakeOption -DAPACHE_DIR  "${APACHE_DIR}"
fi

# qt5
AddCMakeOption -DVISUS_GUI "${VISUS_GUI}"
if (( VISUS_GUI == 1 )); then
	AddCMakeOption -DQt5_DIR "${Qt5_DIR}"
fi

# compile
if (( 1 == 1 )) ; then
	BeginSection "Compiling OpenVisus"
	cmake ${cmake_opts[@]} ${SOURCE_DIR}
	cmake --build ./ --target all --config ${CMAKE_BUILD_TYPE}
fi

# install
if (( 1 == 1 )) ; then
	BeginSection "Installing OpenVisus"	
	cmake --build . --target install --config ${CMAKE_BUILD_TYPE}

	# fix permissions (cmake bug)
	pushd ${CMAKE_BUILD_TYPE}/OpenVisus
	chmod a+rx *.sh 1>/dev/null 2>/dev/null && :
	popd

fi

# dist
if (( DEPLOY_PYPI == 1 || DEPLOY_GITHUB == 1 )) ; then
	BeginSection "CMake dist step"
	cmake --build . --target DIST --config ${CMAKE_BUILD_TYPE}
fi

# cmake tests (dont care if it fails)
if (( VISUS_PYTHON == 1 )) ; then
	BeginSection "Test OpenVisus (cmake)"
	cmake --build  ./ --target  test --config ${CMAKE_BUILD_TYPE}	 && :
fi

# cmake external app (dont care if it fails)
if (( 1 == 1 )) ; then
	BeginSection "Test OpenVisus (cmake external app)"
	cmake --build      ./ --target simple_query    --config ${CMAKE_BUILD_TYPE}  && :
	if (( VISUS_GUI == 1 )) ; then
		cmake --build   ./ --target simple_viewer2d --config ${CMAKE_BUILD_TYPE}  && :
	fi	
fi

cd ${HOME}

# test extending python
if (( VISUS_PYTHON == 1  )) ; then
	BeginSection "Test OpenVisus (extending python)"
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}
	${PYTHON_EXECUTABLE} -m OpenVisus dirname
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	${PYTHON_EXECUTABLE} Samples/python/Array.py
	${PYTHON_EXECUTABLE} Samples/python/Dataflow.py
	${PYTHON_EXECUTABLE} Samples/python/Idx.py
	unset PYTHONPATH
	popd
fi

# test embedding python
if (( VISUS_PYTHON == 1 )) ; then
	BeginSection "Test OpenVisus (embedding python)"
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	./visus.sh
	unset PYTHONPATH
	popd
fi

# deploy pypi
if (( DEPLOY_PYPI == 1 )) ; then
	WHEEL_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/dist -iname "*.whl")
	echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
	echo [distutils]                                  > ~/.pypirc
	echo index-servers =  pypi                       >> ~/.pypirc
	echo [pypi]                                      >> ~/.pypirc
	echo username=${PYPI_USERNAME}                   >> ~/.pypirc
	echo password=${PYPI_PASSWORD}                   >> ~/.pypirc
	${PYTHON_EXECUTABLE} -m twine upload --skip-existing "${WHEEL_FILENAME}"
fi

# deploy github
if (( DEPLOY_GITHUB == 1 )) ; then

	filename=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/dist -iname "*.tar.gz")

	old_filename=$filename
	filename=${filename/manylinux1_x86_64.tar.gz/${OS_NICKNAME}.tar.gz}
	mv $old_filename $filename

	response=$(curl -sH "Authorization: token ${GITHUB_API_TOKEN}" https://api.github.com/repos/sci-visus/OpenVisus/releases/tags/${TRAVIS_TAG})
	eval $(echo "$response" | grep -m 1 "id.:" | grep -w id | tr : = | tr -cd '[[:alnum:]]=')

	curl --data-binary @"${filename}" \
		-H "Authorization: token $GITHUB_API_TOKEN" \
		-H "Content-Type: application/octet-stream" \
		"https://uploads.github.com/repos/sci-visus/OpenVisus/releases/$id/assets?name=$(basename ${filename})"
fi

echo "OpenVisus CMake/build.sh finished"




