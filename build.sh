#!/bin/bash

# stop on error
set -e

# very verbose
set -x

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}

# cmake flags
PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
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
PYPI_PASSWORD=${PYPI_PASSWORD:-}NO_CMAKE_SYSTEM_PATH 

# if is docker or not
DOCKER=0
grep 'docker\|lxc' /proc/1/cgroup && :
if [ $? == 0 ] ; then 
	DOCKER=1
fi



# in case you want to try manylinux-like compilation
SimulateManyLinux=0

# in case you want to speed up compilation because prerequisites have already been installed
FastMode=${FastMode:-0}

OpenVisusCache=${OpenVisusCache:-${BUILD_DIR}/.cache}
mkdir -p ${OpenVisusCache}
export PATH=${OpenVisusCache}/bin:$PATH

if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
	TRAVIS=1
fi

# sudo allowed or not (in general I assume I cannot use sudo)
CanSudo=${CanSudo:-0}
if (( "$EUID" == 0 || DOCKER == 1 || TRAVIS == 1 )); then 
	CanSudo=1
fi

# ///////////////////////////////////////////////////////////////////////////////////////////////
# travis preamble
if (( TRAVIS == 1)) ; then

	if [[ "$TRAVIS_OS_NAME" == "osx"   ]]; then 
		export COMPILER=clang++ 
		sudo gem install xcpretty 
	fi

	if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then 
		export COMPILER=g++-4.9 
	fi

	if (( USE_CONDA == 1 )) ; then
		if [[ "${TRAVIS_TAG}" != "" ]]; then
			export DEPLOY_CONDA=1
		fi
	else
	  DEPLOY_GITHUB=1
	  if [[ "${TRAVIS_TAG}" != "" ]]; then
		  if [[ $(uname) == "Darwin" || "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]]; then
			  export DEPLOY_PYPI=1
		  fi     
	  fi
	fi
fi



# //////////////////////////////////////////////////////
function DownloadFile {

	set +x

	url=$1

	filename=$(basename $url)
	if [ -f "${filename}" ] ; then
		echo "file $filename already downloaded"
		set -x
		return 0	
	fi

	set -x
	curl -fsSL --insecure "$url" -O
	set -x
}


# //////////////////////////////////////////////////////
function GetVersionFromCommand {
	# return the next word after the pattern and parse the version in the format MAJOR.MINOR.whatever
	cmd=$1
	pattern=$2
	__content__=$(${cmd})
	__version__=$(echo ${__content__} | awk -F "${pattern}" '{print $2}' | cut -d' ' -f1)
	__major__=$(echo ${__version__} | cut -d'.' -f1)
	__minor__=$(echo ${__version__} | cut -d'.' -f2)
}




# ///////////////////////////////////////////////////////////////////////////////////////////////
# detect OS

if [ $(uname) = "Darwin" ]; then
	echo "Detected OSX"
	OSX=1

elif [ -x "$(command -v apt-get)" ]; then

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

	echo "Detected ubuntu ${UBUNTU_VERSION}"


elif [ -x "$(command -v zypper)" ]; then
	echo "Detected opensuse"
	OPENSUSE=1

elif [ -x "$(command -v yum)" ]; then

	CENTOS=1

	GetVersionFromCommand "cat /etc/redhat-release" "CentOS release "
	echo "Detected centos ${__version__}"
	if (( ${__major__}  == 5 )) ; then
		MANYLINUX=1
		DISABLE_OPENMP=1
		VISUS_GUI=0
	fi	

else
	echo "Failed to detect OS version, I will keep going but it could be that I won't find some dependency"
fi


# ///////////////////////////////////////////////////////
# Docker-build

if [[ "$DOCKER_IMAGE" != "" ]] ; then

	sudo  docker rm -f mydocker 2>/dev/null || true

	sudo  docker run -d -ti \
		--name mydocker \
		-v ${SOURCE_DIR}:${SOURCE_DIR} \
		-e BUILD_DIR=${BUILD_DIR} \
		-e OpenVisusCache=${OpenVisusCache} \
		-e PYTHON_VERSION=${PYTHON_VERSION} \
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

	sudo  docker exec mydocker /bin/bash -c "cd ${SOURCE_DIR} && ./build.sh"

	sudo  chown -R "$USER":"$USER" ${BUILD_DIR}
	sudo  chmod -R u+rwx           ${BUILD_DIR}
	exit 0
fi



# ///////////////////////////////////////////////////////
# conda-build

if (( USE_CONDA == 1 )) ; then
	
	if (( FASTMODE == 0 )) ; then

		# here I need sudo! 
		if (( OSX ==  1 && CanSudo == 1 )) ; then
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
		
		conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
		conda install -q conda-build anaconda-client
		conda update  -q conda conda-build
		conda install -q python=${PYTHON_VERSION}	
	fi

	export PATH="$HOME/miniconda${PYTHON_VERSION:0:1}/bin:$PATH"
	hash -r

	pushd conda
	conda-build -q openvisus
	conda install -q --use-local openvisus
	popd

	# test openvisus
	cd $(python -m OpenVisus dirname)
	python Samples/python/Array.py
	python Samples/python/Dataflow.py
	python Samples/python/Idx.py

	# deploy to conda
	if (( DEPLOY_CONDA == 1 )) ; then
		CONDA_BUILD_FILENAME=$(find ${HOME}/miniconda${PYTHON_VERSION:0:1}/conda-bld -iname "openvisus*.tar.bz2")
		echo "Doing deploy to anaconda ${CONDA_BUILD_FILENAME}..."
		anaconda -q -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
	fi

	echo "OpenVisus build finished"
	exit 0
fi


# //////////////////////////////////////////////////////
function InstallPackages {

	set +x

	if (( OSX == 1 )) ; then
		CheckInstallCommand="brew list"
		InstallCommand="brew install"

	elif (( UBUNTU == 1 )); then
		CheckInstallCommand="dpkg -s"
		InstallCommand="sudo apt-get -qq install --allow-unauthenticated"

	elif (( OPENSUSE == 1 )) ; then
		CheckInstallCommand=rpm -q
		InstallCommand="sudo zypper --non-interactive install "
		
	elif (( CENTOS == 1 )) ; then
		CheckInstallCommand="yum list installed ${package_name}"
		InstallCommand="sudo yum install -y"

	fi

	AlreadyInstalled=1
	for package_name in $@ ; do
		set +e
		$CheckInstallCommand ${package_name} 1>/dev/null 2>/dev/null && : 
		if [ $? != 0 ] ; then 
			AlreadyInstalled=0
		fi
	done

	if (( AlreadyInstalled == 1 )) ; then
		echo "Already installed: $@"
		set -x
		return 0
	fi

	if [[ $InstallCommand == *"sudo"* && "${CanSudo}" == "0" ]]; then
		echo "Failed to install because I need sudo: $@"
		set -x
		return 1
	fi

	set -x
	$InstallCommand $@  && : 
	retcode=$?
	if ((  retcode == 0 )) ; then 
		echo "Just installed: $@"
	else
		echo "Failed to install: $@"
	fi
	return $retcode
}


# //////////////////////////////////////////////////////////////
function InstallPrerequisites {

	# make sure sudo is available
	if [[ "$CanSudo" == "1" && ! -x "$(command -v sudo)" && "$EUID" -eq 0 ]]; then

		if (( UBUNTU ==1 )); then
			apt-get -qq update
			apt-get -qq install sudo

		elif (( OPENSUSE == 1 )); then
			apt-get -qq update
			apt-get -qq install sudo

		elif (( CENTOS == 1 )); then
			yum update
			yum install sudo

		fi
	fi

	if (( OSX == 1 )) ; then

		if (( FastMode == 0 )) ; then

			# install brew
			if [ !  -x "$(command -v brew)" ]; then
				/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
			fi

			# output is very long!
			brew update 1>/dev/null 2>&1 || true
		fi

		return 0
	fi


	if (( UBUNTU == 1 )) ; then
	
		if (( CanSudo == 1 && FastMode == 0 )) ; then

			sudo apt-get -qq update

			InstallPackages software-properties-common

			if (( ${UBUNTU_VERSION:0:2}<=14 )); then
				sudo add-apt-repository -y ppa:deadsnakes/ppa
				sudo apt-get -qq update
			fi

		fi

		InstallPackages build-essential
		InstallPackages git curl
		InstallPackages ca-certificates uuid-dev automake bzip2
		return 0
	fi

	if (( OPENSUSE == 1 )) ; then

		if (( CanSudo == 1 && FastMode == 0 )) ; then
			sudo zypper --non-interactive update
			sudo zypper --non-interactive install --type pattern devel_basis
		fi

		InstalPackages gcc-c++
		InstalPackages git curl
		InstalPackages lsb-release libuuid-devel 
		return 0
		
	fi

	if (( CENTOS == 1 )) ; then

		if (( CanSudo == 1 && FastMode == 0 )) ; then
			sudo yum update
		fi

		InstallPackages zlib-devel curl libffi-devel
		return 0
	fi
}

# //////////////////////////////////////////////////////
function InstallCMake {

	if (( OSX == 1 || SimulateManyLinux == 0 )); then

		InstallPackages cmake && :

		# already installed
		if [[ -x "$(command -v cmake)" ]]; then
			GetVersionFromCommand "cmake --version" "cmake version "
			if (( __major__== 3 && __minor__ >= 9 )); then
				echo "Good version: cmake==${__version__}"
				return 0
			else
				echo "Wrong version: cmake==${__version__} "
			fi
		fi 
	fi

	# install from source
	echo "installing cmake from source"
	if [ ! -f "${OpenVisusCache}/bin/cmake" ]; then
		CMAKE_VERSION=3.10.1

		# Error with other  versions: `GLIBC_2.6' not found (required by cmake)
		if (( MANYLINUX == 1 )) ; then  CMAKE_VERSION=3.4.3 ; fi 

		url="https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.tar.gz"
		filename=$(basename ${url})
		DownloadFile "${url}"
		tar xzf ${filename} -C ${OpenVisusCache} --strip-components=1 
		rm -f ${filename}
	fi

	return 0
}


# //////////////////////////////////////////////////////
function InstallSwig {

	if (( OSX == 1 || SimulateManyLinux == 0 )); then

		InstallPackages swig && :

		# already installed
		if [[ -x "$(command -v swig)" ]]; then
			GetVersionFromCommand "swig -version" "SWIG Version "
			if (( __major__>= 3)); then
				echo "Good version: swig==${__version__}"
				cmake_opts+=(-DSWIG_EXECUTABLE=$(which swig))	
				return 0
			else
				echo "Wrong version: swig==${__version__}"
			fi
		fi 

		InstallPackages swig3.0 && :

		# already installed
		if [[ -x "$(command -v swig3.0)" ]]; then
			cmake_opts+=(-DSWIG_EXECUTABLE=$(which swig3.0))	
			return 0
		fi

	fi

	# install from source
	echo "installing swig from source"
	if [ ! -f "${OpenVisusCache}/bin/swig" ]; then
		url="https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd swig-3.0.12
		DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
		./Tools/pcre-build.sh 1>/dev/null
		./configure --prefix=${OpenVisusCache} 1>/dev/null && make -s -j 4 1>/dev/null && make install 1>/dev/null 
		popd
		rm -Rf swig-3.0.12
	fi

	cmake_opts+=(-DSWIG_EXECUTABLE=${OpenVisusCache}/bin/swig)
	return 0
}

# //////////////////////////////////////////////////////
function InstallPatchElf {

	if (( OSX == 1 )); then
		return 0
	fi

	if (( SimulateManyLinux == 0 )); then

		InstallPackages patchelf && :

		# already installed
		if [ -x "$(command -v patchelf)" ]; then
			echo "Already installed: patchelf"
			return 0
		fi
	fi

	# install from source
	echo "installing patchelf from source"
	if [ ! -f "${OpenVisusCache}/bin/patchelf" ]; then
		url="https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd patchelf-0.9
		./configure --prefix=${OpenVisusCache} 1>/dev/null && make -s 1>/dev/null && make install 1>/dev/null 
		autoreconf -f -i
		./configure --prefix=${OpenVisusCache} 1>/dev/null && make -s 1>/dev/null && make install 1>/dev/null 
		popd
		rm -Rf patchelf-0.9
	fi

	return 0
}

# //////////////////////////////////////////////////////
function InstallOpenSSL {

	if (( OSX == 1 )); then
		InstallPackages openssl@1.1  && : 
		if [ $? == 0 ] ; then return 0 ; fi
	fi

	if (( SimulateManyLinux == 0 )); then


		if (( UBUNTU == 1 )) ; then
			InstallPackages libssl-dev  && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( OPENSUSE == 1 )) ; then
			InstallPackages libopenssl-devel  && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( CENTOS == 1 )) ; then
			# for centos I prefer to build from scratch
			echo "Centos, prefer source openssl"
		fi
	fi

	# install from source
	echo "installing openssl from source"
	if [ ! -f "${OpenVisusCache}/bin/openssl" ]; then
		url="https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd openssl-1.0.2a
		./config --prefix=${OpenVisusCache} -fpic shared 1>/dev/null && make -s 1>/dev/null  && make install 1>/dev/null 
		popd
		rm -Rf openssl-1.0.2a
	fi

	OPENSSL_INCLUDE_DIR="${OpenVisusCache}/include" 
	OPENSSL_LIB_DIR="${OpenVisusCache}/lib"

	return 0
}


# //////////////////////////////////////////////////////
function InstallApache {

	if (( OSX == 1 )); then
		return 0
	fi

	if (( SimulateManyLinux == 0 )); then

		if (( UBUNTU == 1 )); then
			InstallPackages apache2 apache2-dev libffi-dev  && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( OPENSUSE == 1 )); then
			InstallPackages apache2 apache2-devel libffi-devel  && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( CENTOS == 1 )) ; then
			# for centos I prefer to build from scratch
			echo "centos, prefers source apache"
		fi

	fi

	# install from source
	echo "installing apache from source"

	# install apr 
	if [ !  -f "${OpenVisusCache}/lib/libapr-1.a" ] ; then
		url="http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd apr-1.6.5
		./configure --prefix=${OpenVisusCache} 1>/dev/null && make -s 1>/dev/null && make install 1>/dev/null 
		popd
		rm -Rf apr-1.6.5
	fi

	# install apr utils 
	if [ !  -f "${OpenVisusCache}/lib/libaprutil-1.a" ] ; then
		url="http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd apr-util-1.6.1
		./configure --prefix=${OpenVisusCache} --with-apr=${OpenVisusCache} 1>/dev/null  && make -s 1>/dev/null  && make install 1>/dev/null 
		popd
		rm -Rf apr-util-1.6.1
	fi

	# install pcre 
	if [ !  -f "${OpenVisusCache}/lib/libpcre.a" ] ; then
		url="https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd pcre-8.42
		./configure --prefix=${OpenVisusCache} 1>/dev/null  && make -s 1>/dev/null  && make install 1>/dev/null 
		popd
		rm -Rf pcre-8.42
	fi

	# install httpd
	if [ !  -f "${OpenVisusCache}/include/httpd.h" ] ; then
		url="http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz"
		filename=$(basename ${url})
		DownloadFile ${url}
		tar xzf ${filename}
		pushd httpd-2.4.38
		./configure --prefix=${OpenVisusCache} --with-apr=${OpenVisusCache} --with-pcre=${OpenVisusCache} --with-ssl=${OpenVisusCache} 1>/dev/null && \
			make -s 1>/dev/null && \
			make install 1>/dev/null 
		popd
		rm -Rf httpd-2.4.38
	fi
	
	cmake_opts+=(-DAPR_DIR=${OpenVisusCache})
	cmake_opts+=(-DAPACHE_DIR=${OpenVisusCache})
	return 0
}


# //////////////////////////////////////////////////////
function InstallQt5 {

	# no need
	if (( VISUS_GUI != 1 )); then
		return 0
	fi

	# already set by user
	if [ ! -z "${Qt5_DIR}" ] ; then
		cmake_opts+=(-DQt5_DIR=${Qt5_DIR})
		return 0
	fi

	# install qt 5.11 (instead of 5.12 which is not supported by PyQt5)
	if (( OSX == 1 )); then

		if (( FastMode == 0 )) ; then
			echo "installing Qt5 from brew"
			brew uninstall qt5 1>/dev/null 2>/dev/null && :
			InstallPackages https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb
		fi

		Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5
		cmake_opts+=(-DQt5_DIR=${Qt5_DIR})
		return 0
	fi

	if (( SimulateManyLinux == 0 )); then

		# ubuntu
		if (( UBUNTU == 1 )) ; then

			# https://launchpad.net/~beineri
			# PyQt5 versions 5.6, 5.7, 5.7.1, 5.8, 5.8.1.1, 5.8.2, 5.9, 5.9.1, 5.9.2, 5.10, 5.10.1, 5.11.2, 5.11.3
			if (( ${UBUNTU_VERSION:0:2} <=14 )); then
				QT5_PACKAGE=qt510base
				QT5_REPOSITORY=ppa:beineri/opt-qt-5.10.1-trusty
				OPT_QT5_DIR=/opt/qt510/lib/cmake/Qt5

			elif (( ${UBUNTU_VERSION:0:2} <=16 )); then
				QT5_PACKAGE=qt511base
				QT5_REPOSITORY=ppa:beineri/opt-qt-5.11.2-xenial
				OPT_QT5_DIR=/opt/qt511/lib/cmake/Qt5

			elif (( ${UBUNTU_VERSION:0:2} <=18)); then
				QT5_PACKAGE=qt511base
				QT5_REPOSITORY=ppa:beineri/opt-qt-5.11.2-bionic
				OPT_QT5_DIR=/opt/qt511/lib/cmake/Qt5

			else
				InternalError
			fi

			if (( CanSudo == 1 )) ; then
				if (( FastMode== 0 )) ; then
					sudo add-apt-repository ${QT5_REPOSITORY} -y 
					sudo apt-get -qq update
				fi
			fi

			InstallPackages mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev ${QT5_PACKAGE}  && : 
			if [ $? == 0 ] ; then
				echo "Using Qt5 from unbuntu repository"
				Qt5_DIR=${OPT_QT5_DIR}
				cmake_opts+=(-DQt5_DIR=${Qt5_DIR})
				return 0
			fi
		fi

		# opensuse
		if (( OPENSUSE == 1 )) ; then
			InstallPackages glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel && : 
			if [ $? == 0 ] ; then return 0 ; fi
		fi

	fi

	# backup plan , use a minimal Qt5 which does not need SUDO
	# You can use your Qt5 by setting Qt5_DIR in command line 
	# Default is to use a "minimal" Qt I'm storing on atlantis (NOTE: it does not contains *.so files or plugins, I'm using PyQt5 for that)
	
	echo "Using minimal Qt5"

	QT_VERSION=5.11.2
	Qt5_DIR=${OpenVisusCache}/qt${QT_VERSION}/lib/cmake/Qt5
	cmake_opts+=(-DQt5_DIR=${Qt5_DIR})

	# if you want to create a "new" minimal Qt5 file this is what I did
	if (( 0 == 1 )); then
		wget http://download.qt.io/official_releases/qt/${QT_VERSION:0:4}/${QT_VERSION}/qt-opensource-linux-x64-${QT_VERSION}.run
		chmod +x qt-opensource-linux-x64-${QT_VERSION}.run
		./qt-opensource-linux-x64-5${QT_VERSION}.run	
		SRC=${HOME}/Qt${QT_VERSION}/${QT_VERSION}/gcc_64
		cd /tmp
		rm -Rf   qt${QT_VERSION}/*
		mkdir -p qt${QT_VERSION}
		mkdir -p qt${QT_VERSION}/bin
		mkdir -p qt${QT_VERSION}/lib
		mkdir -p qt${QT_VERSION}/plugins
		mkdir -p qt${QT_VERSION}/mkspecs
		cp -r ${SRC}/include                 qt${QT_VERSION}/  
		cp -r ${SRC}/bin/qmake               qt${QT_VERSION}/bin/
		cp -r ${SRC}/bin/moc                 qt${QT_VERSION}/bin/
		cp -r ${SRC}/bin/rcc                 qt${QT_VERSION}/bin/
		cp -r ${SRC}/bin/uic                 qt${QT_VERSION}/bin/ 
		cp -r ${SRC}/lib/cmake               qt${QT_VERSION}/lib/
		cp -r ${SRC}/mkspecs/linux-g++       qt${QT_VERSION}/mkspecs/
		tar cvzf qt${QT_VERSION}.tar.gz qt${QT_VERSION}
		scp qt${QT_VERSION}.tar.gz scrgiorgio@atlantis.sci.utah.edu:/www/qt/
		rm -Rf qt${QT_VERSION}.tar.gz qt${QT_VERSION}
	fi

	if [ ! -d "${Qt5_DIR}" ] ; then

		url="http://atlantis.sci.utah.edu/qt/qt${QT_VERSION}.tar.gz"
		filename=$(basename ${url})
		DownloadFile "${url}"
		tar xzf ${filename} -C ${OpenVisusCache} 

		python -m pip install -q uninstall PyQt5 || true
		python -m pip install -q --user PyQt5==${QT_VERSION}
		PyQt5_DIR=$(python -c 'import os,PyQt5;print(os.path.dirname(PyQt5.__file__))')

		cp -r ${PyQt5_DIR}/Qt/lib/*   ${OpenVisusCache}/qt${QT_VERSION}/lib/
		cp -r ${PyQt5_DIR}/Qt/plugins ${OpenVisusCache}/qt${QT_VERSION}/

		# fix *.so links (example libQt5OpenGL.so.5.11.2 / libQt5OpenGL.so.5.11 / libQt5OpenGL.so.5)
		FILES=$(find ${OpenVisusCache}/qt${QT_VERSION}/lib -iname "libQt5*.so.5")
		for it in ${FILES}
		do
			ln -s ${it} ${it}.${QT_VERSION:2:2}
			ln -s ${it} ${it}.${QT_VERSION:2:2}.${QT_VERSION:5:1}
		done

	fi
	return 0
}


# ///////////////////////////////////////////////////////
function InstallPython {

	# install python using pyenv
	if (( FastMode == 0 )) ; then
	
		if (( OSX == 1 )) ; then

			InstallPackages readline zlib
			InstallPackages pyenv

			# this avoid problems
			brew upgrade readline zlib pyenv 1>/dev/null && :
		
			CONFIGURE_OPTS="--enable-shared --with-openssl=$(brew --prefix openssl@1.1)" \
			CFLAGS=" -I$(brew --prefix readline)/include -I$(brew --prefix zlib)/include  -I$(brew --prefix openssl@1.1)/include" \
			LDFLAGS="-L$(brew --prefix readline)/lib     -L$(brew --prefix zlib)/lib      -L$(brew --prefix openssl@1.1)/lib" \
			pyenv install --skip-existing ${PYTHON_VERSION}
			if [ $? != 0 ] ; then
				echo "Installation of python failed"
				exit -1
			fi

		else

			if ! [ -d "$HOME/.pyenv" ]; then
				pushd $HOME
				DownloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
				chmod a+x pyenv-installer
				./pyenv-installer
				rm -f pyenv-installer
				popd
			fi
		
			# activate pyenv
			export PATH="$HOME/.pyenv/bin:$PATH"
			eval "$(pyenv init -)"

			declare -a __opt__
			__opt__+=(CXX=g++)
			__opt__+=(CONFIGURE_OPTS="--enable-shared")

			if [[ "$OPENSSL_DIR" != "" ]] ; then
				__opt__+=(--with-openssl=${OPENSSL_DIR})
				__opt__+=(CFLAGS="-I${OPENSSL_DIR}/include")
				__opt__+=(LDFLAGS="-L${OPENSSL_DIR}/lib")
			fi

			${__opt__[@]} pyenv install --skip-existing ${PYTHON_VERSION} && :
			if [ $? != 0 ] ; then
				echo "Installation of python failed"
				exit -1
			fi
			
		
		fi
	fi

	# activate pyenv
	if (( OSX != 1 )) ; then
		export PATH="$HOME/.pyenv/bin:$PATH"	
	fi

	eval "$(pyenv init -)"
	pyenv global ${PYTHON_VERSION}
	pyenv rehash

	# install python packages
	if (( FastMode == 0 )) ; then	
		python -m pip install -q --upgrade pip
		python -m pip install -q numpy setuptools wheel twine auditwheel	
	fi

	if [ "${PYTHON_VERSION:0:1}" -gt "2" ]; then
		__m__=m
	else
		__m__=
	fi

	if (( OSX == 1 )) ; then
		__ext__=.dylib
	else
		__ext__=.so
	fi

	cmake_opts+=(-DPYTHON_EXECUTABLE=$(pyenv prefix)/bin/python)
	cmake_opts+=(-DPYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_VERSION:0:3}${__m__})
	cmake_opts+=(-DPYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_VERSION:0:3}${__m__}${__ext__})
	cmake_opts+=(-DPYTHON_VERSION=${PYTHON_VERSION})

	return 0
}


mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

declare -a cmake_opts

cmake_opts+=(-DDISABLE_OPENMP=${DISABLE_OPENMP})
cmake_opts+=(-DVISUS_GUI=${VISUS_GUI})
cmake_opts+=(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

InstallPrerequisites
InstallOpenSSL
InstallCMake
InstallSwig
InstallPatchElf
InstallApache
InstallQt5
InstallPython

if (( OSX == 1 )) ; then
	cmake -GXcode ${cmake_opts[@]} ${SOURCE_DIR}
	 
	# this is to solve logs too long 
	if (( TRAVIS == 1 )) ; then 
		cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
	else
		cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE}
	fi	
	
else
	cmake ${cmake_opts[@]} ${SOURCE_DIR}
	cmake --build . --target all -- -j 4
	
fi

cmake --build . --target install --config ${CMAKE_BUILD_TYPE}


# dist
if (( DEPLOY_GITHUB == 1 || DEPLOY_PYPI == 1 )) ; then

	cmake --build . --target dist --config ${CMAKE_BUILD_TYPE}

	if (( DEPLOY_PYPI == 1 )) ; then
		WHEEL_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.whl")
		echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
		echo [distutils]                                  > ~/.pypirc
		echo index-servers =  pypi                       >> ~/.pypirc
		echo [pypi]                                      >> ~/.pypirc
		echo username=${PYPI_USERNAME}                   >> ~/.pypirc
		echo password=${PYPI_PASSWORD}                   >> ~/.pypirc
		python -m twine upload --skip-existing "${WHEEL_FILENAME}"
	fi

fi

# tests using CMake targets
if (( 1 == 1 )) ; then
	if (( OSX == 1 )) ; then
		cmake --build   . --target  RUN_TESTS       --config ${CMAKE_BUILD_TYPE}	
	else
		cmake --build   . --target  test            --config ${CMAKE_BUILD_TYPE}	
	fi
	
	# test external app
	cmake --build      . --target  simple_query    --config ${CMAKE_BUILD_TYPE}
	
	if (( VISUS_GUI == 1 )) ; then
		cmake --build   . --target  simple_viewer2d --config ${CMAKE_BUILD_TYPE}
	fi
fi

# test OpenVisus package using python
if (( 1 == 1 )); then
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages
	cd $(python -m OpenVisus dirname)
	python Samples/python/Array.py
	python Samples/python/Dataflow.py
	python Samples/python/Idx.py
	
	# test stand alone scripts
	python -m OpenVisus CreateScripts
	if (( OSX == 1 )) ; then
		./visus.command
	else
		./visus.sh
	fi
fi

echo "OpenVisus build finished"


