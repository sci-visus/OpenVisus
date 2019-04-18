#!/bin/bash


# very verbose
set -x

# stop on error
set -e

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}
CACHE_DIR=${CACHE_DIR:-${BUILD_DIR}/.cache}
PYENV_ROOT=${PYENV_ROOT:-${HOME}/.pyenv}

# cmake flags
PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

# conda stuff
USE_CONDA=${USE_CONDA:-0}
DEPLOY_CONDA=${DEPLOY_CONDA:-0}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}


# in case you want to try manylinux-like compilation
UseInstalledPackages=${UseInstalledPackages:-1}

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

	curl -fsSL --insecure "$url" -O
	set -x
}


# //////////////////////////////////////////////////////
# return the next word after the pattern and parse the version in the format MAJOR.MINOR.whatever
function GetVersionFromCommand {	
	set +x
	__version__=""
	__major__=""
	__minor__=""
	__patch__=""
	cmd=$1
	pattern=$2

	set +e
	__content__=$(${cmd} 2>/dev/null)
	retcode=$?
	set -e

	if [ $? == 0 ] ; then 
		__version__=$(echo ${__content__} | awk -F "${pattern}" '{print $2}' | cut -d' ' -f1)
		__major__=$(echo ${__version__} | cut -d'.' -f1)
		__minor__=$(echo ${__version__} | cut -d'.' -f2)
		__patch__=$(echo ${__version__} | cut -d'.' -f3)
		echo "Found version ${__version__}"
	else
		echo "Cannot find any version"
	fi
	set -x
}


# ///////////////////////////////////////////////////////////////////////////////////////////////
function EchoSection {
	set +x	
	echo "//////////////////////////////////////////////////////////////////////// $1"
	set -x
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function BuildPreamble {	

	# if is docker or not
	DOCKER=0
	grep 'docker\|lxc' /proc/1/cgroup && :
	if [ $? == 0 ] ; then 
		export DOCKER=1
	fi

	# If is travis or not 
	if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
		export TRAVIS=1 

		if (( USE_CONDA == 1 )) ; then
			if [[ "${TRAVIS_TAG}" != "" ]]; then
				export DEPLOY_CONDA=1
			fi
		fi
	fi

	if [ $(uname) = "Darwin" ]; then
		OSX=1
		echo "Detected OSX"

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
		OPENSUSE=1
		echo "Detected opensuse"

	elif [ -x "$(command -v yum)" ]; then
		CENTOS=1
		GetVersionFromCommand "cat /etc/redhat-release" "CentOS release "
		CENTOS_MAJOR=${__major__}
		echo "Detected centos ${__version__}"

	else
		echo "Failed to detect OS version, I will keep going but it could be that I won't find some dependency"
	fi

	# sudo allowed or not (in general I assume I cannot use sudo)
	SudoCmd="sudo"
	IsRoot=${IsRoot:-0}
	if (( EUID== 0 || DOCKER == 1 || TRAVIS == 1 )); then 
		IsRoot=1
		SudoCmd=""
	fi
}


# ///////////////////////////////////////////////////////////////////////////////////////////////
function DockerBuild {

	# note: sudo is needed anyway otherwise travis fails
	sudo docker rm -f mydocker 2>/dev/null || true

	declare -a docker_opts

	docker_opts+=(-v ${SOURCE_DIR}:/root/OpenVisus)
	docker_opts+=(-e SOURCE_DIR=/root/OpenVisus)

	#docker_opts+=(-v ${BUILD_DIR}:/root/.build)
	#docker_opts+=(-e BUILD_DIR=/root/.build)

	#docker_opts+=(-v ${CACHE_DIR}:/root/.cache)
	#docker_opts+=(-e CACHE_DIR=/root/.cache)

	#docker_opts+=(-v ${PYENV_ROOT}:/root/.pyenv)
	#docker_opts+=(-e ${PYENV_ROOT}=/root/.pyenv)

	docker_opts+=(-e PYTHON_VERSION=${PYTHON_VERSION})
	docker_opts+=(-e DISABLE_OPENMP=${DISABLE_OPENMP})
	docker_opts+=(-e VISUS_GUI=${VISUS_GUI})
	docker_opts+=(-e CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
	docker_opts+=(-e USE_CONDA=${USE_CONDA})
	docker_opts+=(-e DEPLOY_CONDA=${DEPLOY_CONDA})
	docker_opts+=(-e ANACONDA_TOKEN=${ANACONDA_TOKEN})

	sudo docker run -d -ti --name mydocker ${docker_opts[@]} ${DOCKER_IMAGE} /bin/bash
	sudo docker exec mydocker /bin/bash -c "cd /root/OpenVisus && ./build.sh"

	sudo chown -R "$USER":"$USER" ${BUILD_DIR} 1>/dev/null && :
	sudo chmod -R u+rwx           ${BUILD_DIR} 1>/dev/null && :
}



# ///////////////////////////////////////////////////////////////////////////////////////////////
function CondaBuild {

	# here I need sudo! 
	if (( OSX ==  1 && IsRoot == 1 )) ; then
		if [ ! -d /opt/MacOSX10.9.sdk ] ; then
			git clone https://github.com/phracker/MacOSX-SDKsF
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
	conda install -q conda-build anaconda-client && :
	conda update  -q conda conda-build           && :
	conda install -q python=${PYTHON_VERSION}	   && :

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
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
	fi
}


# //////////////////////////////////////////////////////
function InstallPackages {

	set +x

	packages=$@
	echo "Installing packages ${packages}..."

	if (( OSX == 1 )) ; then
		CheckInstallCommand="brew list"
		InstallCommand="brew install"

	elif (( UBUNTU == 1 )); then
		CheckInstallCommand="dpkg -s"
		InstallCommand="${SudoCmd} apt-get -qq install --allow-unauthenticated"

	elif (( OPENSUSE == 1 )) ; then
		CheckInstallCommand="rpm -q"
		InstallCommand="${SudoCmd} zypper --non-interactive install"
		
	elif (( CENTOS == 1 )) ; then
		CheckInstallCommand="yum list installed"
		InstallCommand="${SudoCmd} yum install -y"

	fi

	AlreadyInstalled=1
	for package_name in ${packages} ; do
		$CheckInstallCommand ${package_name} 1>/dev/null 2>/dev/null && : 
		retcode=$?
		if [ ${retcode} != 0 ] ; then 
			AlreadyInstalled=0
		fi
	done

	if (( AlreadyInstalled == 1 )) ; then
		echo "Already installed: ${packages}"
		set -x
		return 0
	fi

	if [[ "${SudoCmd}" != "" && ${InstallCommand} == *"${SudoCmd}"* && "${IsRoot}" == "0" ]]; then
		echo "Failed to install because I need ${SudoCmd}: ${packages}"
		set -x
		return 1
	fi

	$InstallCommand ${packages}  1>/dev/null && : 
	retcode=$?
	if ((  retcode == 0 )) ; then 
		echo "Installed packages: ${packages}"
	else
		echo "Failed to install: ${packages}"
	fi

	set -x
	return $retcode
}



# //////////////////////////////////////////////////////////////
function UpdateOSAndInstallCompilers {

	echo "Installing prerequisites..."

	if (( OSX == 1 )) ; then

		#  for travis long log
		if (( TRAVIS == 1 )) ; then
			${SudoCmd} gem install xcpretty 
		fi 

		# install brew
		if [ !  -x "$(command -v brew)" ]; then
			/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
		fi

		# output is very long!
		brew update 1>/dev/null && :

		InstallPackages libffi

	elif (( UBUNTU == 1 )) ; then
	
		if (( IsRoot == 1 )) ; then
			${SudoCmd} apt-get -qq update 1>/dev/null  && :
		fi

		# install additional rep
		if (( IsRoot == 1 )) ; then
			InstallPackages software-properties-common
			if (( ${UBUNTU_VERSION:0:2}<=14 )); then
				${SudoCmd} add-apt-repository -y ppa:deadsnakes/ppa
				${SudoCmd} apt-get -qq update
			fi
		fi

		InstallPackages build-essential git curl ca-certificates uuid-dev automake bzip2 libffi-dev

	elif (( OPENSUSE == 1 )) ; then

		if (( IsRoot == 1 )) ; then
			${SudoCmd} zypper --non-interactive update 1>/dev/null  && :
			${SudoCmd} zypper --non-interactive install --type pattern devel_basis
		fi

		InstallPackages gcc-c++ git curl lsb-release libuuid-devel libffi-devel
		
	elif (( CENTOS == 1 )) ; then

		if (( IsRoot == 1 )) ; then
			${SudoCmd} yum update 1>/dev/null  && :
		fi

		InstallPackages gcc-c++ zlib zlib-devel curl libffi-devel
	fi

	
	echo "Installed prerequisites"
}

# //////////////////////////////////////////////////////
function InstallCMake {

	if [ -f "${CACHE_DIR}/bin/cmake" ]; then
		echo "Using cached cmake"
		return 0
	fi

	if (( OSX == 1 || UseInstalledPackages == 1 )); then

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

	echo "installing cached cmake"
	
	if (( CENTOS == 1 && CENTOS_MAJOR <= 5 )) ; then  
		__version__=3.4.3  # Error with other  versions: `GLIBC_2.6' not found (required by cmake)
	else
		__version__=3.10.1
	fi 

	url="https://github.com/Kitware/CMake/releases/download/v${__version__}/cmake-${__version__}-Linux-x86_64.tar.gz"
	filename=$(basename ${url})
	DownloadFile "${url}"
	tar xzf ${filename} -C ${CACHE_DIR} --strip-components=1 
	rm -f ${filename}
	return 0
}


# //////////////////////////////////////////////////////
function InstallSwig {

	if [ -f "${CACHE_DIR}/bin/swig" ]; then	
		echo "Using cached swig"
		cmake_opts+=(-DSWIG_EXECUTABLE=${CACHE_DIR}/bin/swig)
		return 0
	fi

	if (( OSX == 1 || UseInstalledPackages == 1 )); then

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

	echo "installing cached swig"
	url="https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd swig-3.0.12
	DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
	./Tools/pcre-build.sh 1>/dev/null
	./configure --prefix=${CACHE_DIR} 1>/dev/null && make -s -j 4 1>/dev/null && make install 1>/dev/null 
	popd
	rm -Rf swig-3.0.12

	cmake_opts+=(-DSWIG_EXECUTABLE=${CACHE_DIR}/bin/swig)
	return 0
}

# //////////////////////////////////////////////////////
function InstallPatchElf {

	if [ -f "${CACHE_DIR}/bin/patchelf" ]; then
		echo "using cached patchelf"
		return 0
	fi

	if (( UseInstalledPackages == 1 )); then

		InstallPackages patchelf && :

		# already installed
		if [ -x "$(command -v patchelf)" ]; then
			echo "Already installed: patchelf"
			return 0
		fi
	fi

	echo "installing cached patchelf"
	url="https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd patchelf-0.9
	./configure --prefix=${CACHE_DIR} 1>/dev/null && make -s 1>/dev/null && make install 1>/dev/null 
	autoreconf -f -i
	./configure --prefix=${CACHE_DIR} 1>/dev/null && make -s 1>/dev/null && make install 1>/dev/null 
	popd
	rm -Rf patchelf-0.9

	return 0
}

# //////////////////////////////////////////////////////
function CheckOpenSSLVersion {

	GetVersionFromCommand "$1 version" "OpenSSL "

	if [[ "${__major__}" == "" || "${__minor__}" == "" || "${__patch__}" == "" ]]; then
		echo "OpenSSL not found"
		return -1

	# Python requires an OpenSSL 1.0.2 or 1.1 compatible libssl with X509_VERIFY_PARAM_set1_host().
	elif [[ "${__major__}" == "1" && "${__minor__}" == "0" && "${__patch__:0:1}" -lt "2" ]]; then
		echo "OpenSSL version(${__version__}) too old"
		return -1


	else
		echo "Openssl version(${__version__}) ok"
		return 0

	fi
}


# //////////////////////////////////////////////////////
function InstallOpenSSL {

	if (( OSX == 1 )); then
		InstallPackages openssl openssl@1.1  && :
		return 0
	fi

	unset OPENSSL_DIR

	if [ -f "${CACHE_DIR}/bin/openssl" ]; then
		echo "Using cached openssl"
		export OPENSSL_DIR="${CACHE_DIR}" 
		export LD_LIBRARY_PATH="${CACHE_DIR}/lib:${LD_LIBRARY_PATH}"
		return 0
	fi
	
	if (( UseInstalledPackages == 1 )); then

		if (( UBUNTU == 1 )) ; then

			InstallPackages libssl-dev && : 
			if [ $? == 0 ] ; then
				CheckOpenSSLVersion /usr/bin/openssl && : 
				if [ $? == 0 ] ; then return 0 ; fi
			fi

		elif (( OPENSUSE == 1 )) ; then

			InstallPackages libopenssl-devel && : 
			if [ $? == 0 ] ; then
				CheckOpenSSLVersion /usr/bin/openssl && : 
				if [ $? == 0 ] ; then return 0 ; fi
			fi

		elif (( CENTOS == 1 )) ; then
			echo "Centos, prefer source openssl"
		fi

	fi

	echo "installing cached openssl"
	url="https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd openssl-1.0.2a
	./config --prefix=${CACHE_DIR} -fPIC shared 1>/dev/null && make -s 1>/dev/null  && make install 1>/dev/null 
	popd
	rm -Rf openssl-1.0.2a
	
	export OPENSSL_DIR="${CACHE_DIR}" 
	export LD_LIBRARY_PATH="${CACHE_DIR}/lib:${LD_LIBRARY_PATH}"
	return 0
}


# //////////////////////////////////////////////////////
function InstallApache {

	if [ -f "${CACHE_DIR}/include/httpd.h" ] ; then
		echo "Using cached apache"
		cmake_opts+=(-DAPR_DIR=${CACHE_DIR})
		cmake_opts+=(-DAPACHE_DIR=${CACHE_DIR})
		return 0		
	fi

	if (( UseInstalledPackages == 1 )); then

		if (( UBUNTU == 1 )); then
			InstallPackages apache2 apache2-dev   && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( OPENSUSE == 1 )); then
			InstallPackages apache2 apache2-devel   && : 
			if [ $? == 0 ] ; then return 0 ; fi

		elif (( CENTOS == 1 )) ; then
			# for centos I prefer to build from scratch
			echo "centos, prefers source apache"
		fi

	fi

	echo "installing cached apache"

	# install apr 
	url="http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd apr-1.6.5
	./configure --prefix=${CACHE_DIR} 1>/dev/null && make -s 1>/dev/null && make install 1>/dev/null 
	popd
	rm -Rf apr-1.6.5

	# install apr utils 
	url="http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd apr-util-1.6.1
	./configure --prefix=${CACHE_DIR} --with-apr=${CACHE_DIR} 1>/dev/null  && make -s 1>/dev/null  && make install 1>/dev/null 
	popd
	rm -Rf apr-util-1.6.1

	# install pcre 
	url="https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd pcre-8.42
	./configure --prefix=${CACHE_DIR} 1>/dev/null  && make -s 1>/dev/null  && make install 1>/dev/null 
	popd
	rm -Rf pcre-8.42

	# install httpd
	url="http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd httpd-2.4.38
	./configure --prefix=${CACHE_DIR} --with-apr=${CACHE_DIR} --with-pcre=${CACHE_DIR} --with-ssl=${CACHE_DIR} 1>/dev/null && \
		make -s 1>/dev/null && \
		make install 1>/dev/null 
	popd
	rm -Rf httpd-2.4.38
	
	cmake_opts+=(-DAPR_DIR=${CACHE_DIR})
	cmake_opts+=(-DAPACHE_DIR=${CACHE_DIR})
	return 0
}


# //////////////////////////////////////////////////////
function InstallQt5 {

	# already set by user
	if [ ! -z "${Qt5_DIR}" ] ; then
		cmake_opts+=(-DQt5_DIR=${Qt5_DIR})
		return 0
	fi

	# install qt 5.11 (instead of 5.12 which is not supported by PyQt5)
	if (( OSX == 1 )); then

		if [ ! -d /usr/local/Cellar/qt/5.11.2_1 ] ; then
			echo "installing brew Qt5"
			brew uninstall qt5 1>/dev/null 2>/dev/null && :
			InstallPackages "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"
		fi

		Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5
		cmake_opts+=(-DQt5_DIR=${Qt5_DIR})
		return 0
	fi

	if (( UseInstalledPackages == 1 )); then

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

			if (( IsRoot == 1 )) ; then
				${SudoCmd} add-apt-repository ${QT5_REPOSITORY} -y && :
				${SudoCmd} apt-get -qq update && :
			fi

			InstallPackages mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev ${QT5_PACKAGE}  && : 
			if [ $? == 0 ] ; then
				echo "Using Qt5 from unbuntu repository"g
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
	Qt5_DIR=${CACHE_DIR}/qt${QT_VERSION}/lib/cmake/Qt5
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
		tar czf qt${QT_VERSION}.tar.gz qt${QT_VERSION}
		scp qt${QT_VERSION}.tar.gz scrgiorgio@atlantis.sci.utah.edu:/www/qt/
		rm -Rf qt${QT_VERSION}.tar.gz qt${QT_VERSION}
	fi

	if [ ! -d "${Qt5_DIR}" ] ; then

		url="http://atlantis.sci.utah.edu/qt/qt${QT_VERSION}.tar.gz"
		filename=$(basename ${url})
		DownloadFile "${url}"
		tar xzf ${filename} -C ${CACHE_DIR} 
		
		GetVersionFromCommand "python -m pip show PyQt5" "Version: "
		if [[ "${__version__}" != "${QT_VERSION}" ]]; then
			python -m pip install -q uninstall PyQt5 || true
			python -m pip install -q --user PyQt5==${QT_VERSION}
			echo "PyQt5==${QT_VERSION} installed"
		else
			echo "PyQt5==${QT_VERSION} already installed"
		fi

		PyQt5_DIR=$(python -c 'import os,PyQt5;print(os.path.dirname(PyQt5.__file__))')

		cp -r ${PyQt5_DIR}/Qt/lib/*   ${CACHE_DIR}/qt${QT_VERSION}/lib/
		cp -r ${PyQt5_DIR}/Qt/plugins ${CACHE_DIR}/qt${QT_VERSION}/

		# fix *.so links (example libQt5OpenGL.so.5.11.2 / libQt5OpenGL.so.5.11 / libQt5OpenGL.so.5)
		FILES=$(find ${CACHE_DIR}/qt${QT_VERSION}/lib -iname "libQt5*.so.5")
		for it in ${FILES}
		do
			ln -s ${it} ${it}.${QT_VERSION:2:2}
			ln -s ${it} ${it}.${QT_VERSION:2:2}.${QT_VERSION:5:1}
		done

	fi

	return 0
}


# ///////////////////////////////////////////////////////
function InstallPyEnvPython {

	# install python using pyenv
	
	if (( OSX == 1 )) ; then

		InstallPackages readline zlib  pyenv && :

		export CONFIGURE_OPTS="--enable-shared"
		pyenv install --skip-existing ${PYTHON_VERSION} && :
		if [ $? != 0 ] ; then 
			echo "pyenv failed to install"
			pyenv install --list
			exit -1
		fi
		
	else

		if [ ! -f "$HOME/.pyenv/bin/pyenv" ]; then
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

		export CONFIGURE_OPTS="--enable-shared"

		if [[ "$OPENSSL_DIR" != "" ]] ; then
			export CONFIGURE_OPTS="${CONFIGURE_OPTS} --with-openssl=${OPENSSL_DIR}"
			export CFLAGS="  -I${OPENSSL_DIR}/include -I${OPENSSL_DIR}/include/openssl"
			export CPPFLAGS="-I${OPENSSL_DIR}/include -I${OPENSSL_DIR}/include/openssl"
			export LDFLAGS=" -L${OPENSSL_DIR}/lib"
		fi

		CXX=g++ pyenv install --skip-existing ${PYTHON_VERSION}  && :
		if [ $? != 0 ] ; then 
			echo "pyenv failed to install"
			pyenv install --list
			exit -1
		fi

		unset CONFIGURE_OPTS
		unset CFLAGS
		unset CPPFLAGS
		unset LDFLAGS
	
	fi

	# activate pyenv
	if (( OSX != 1 )) ; then
		export PATH="$HOME/.pyenv/bin:$PATH"	
	fi

	eval "$(pyenv init -)"
	pyenv global ${PYTHON_VERSION}
	pyenv rehash

	# install python packages
	python -m pip install -q --upgrade pip
	python -m pip install -q numpy setuptools wheel twine auditwheel	

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


# /////////////////////////////////////////////////////////////////////
EchoSection "BuildPreamble"
BuildPreamble

# minimal (manylinux)
if (( CENTOS == 1 && CENTOS_MAJOR <=5 )); then
	DISABLE_OPENMP=1
	VISUS_GUI=0
	UseInstalledPackages=0
fi

mkdir -p ${BUILD_DIR}
mkdir -p ${CACHE_DIR}

if [[ "$DOCKER_IMAGE" != "" ]] ; then
	EchoSection "DockerBuild"
	DockerBuild
	echo "OpenVisus docker build finished"
	exit 0
fi

if (( USE_CONDA == 1 )) ; then
	EchoSection "CondaBuild"
	CondaBuild
	echo "OpenVisus conda build finished"
	exit 0
fi

EchoSection "PyEnvBuild"
cd ${BUILD_DIR}
export PATH=${CACHE_DIR}/bin:$PATH

declare -a cmake_opts

cmake_opts+=(-DDISABLE_OPENMP=${DISABLE_OPENMP})
cmake_opts+=(-DVISUS_GUI=${VISUS_GUI})
cmake_opts+=(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

EchoSection "UpdateOSAndInstallCompilers"
if [ -f ${BUILD_DIR}/.done.UpdateOSAndInstallCompilers ] ; then
	echo "already installed"
else
	UpdateOSAndInstallCompilers
	touch ${BUILD_DIR}/.done.UpdateOSAndInstallCompilers
fi

EchoSection "InstallCMake"
InstallCMake

EchoSection "InstallSwig"
InstallSwig

if (( OSX != 1 )); then
	EchoSection "InstallPatchElf"
	InstallPatchElf
fi

EchoSection "InstallOpenSSL"
InstallOpenSSL

EchoSection "InstallPyEnvPython"
InstallPyEnvPython

if (( OSX != 1 )); then
	EchoSection "InstallApache"
	InstallApache
fi

if (( VISUS_GUI == 1 )); then
	EchoSection "InstallQt5"
	InstallQt5
fi

EchoSection "Build OpenVisus"
if (( OSX == 1 )) ; then

	cmake -GXcode ${cmake_opts[@]} ${SOURCE_DIR}  

	if (( TRAVIS == 1 )) ; then
		${SudoCmd} gem install xcpretty  
		set -o pipefail && cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
	else
		cmake                    --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE}
	fi	
	
else
	cmake ${cmake_opts[@]} ${SOURCE_DIR}
	cmake --build ./ --target all -- -j 4 
	
fi

EchoSection "Install OpenVisus"
cmake --build . --target install --config ${CMAKE_BUILD_TYPE}


# doploy
EchoSection "dist OpenVisus"
if [[ "$TRAVIS" == "1" && "${TRAVIS_TAG}" != "" ]] ; then

	cmake --build . --target dist --config ${CMAKE_BUILD_TYPE}

	if [[ $(uname) == "Darwin" || "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]]; then
		echo "deploy to pypi"
		WHEEL_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.whl")
		echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
		echo [distutils]                                  > ~/.pypirc
		echo index-servers =  pypi                       >> ~/.pypirc
		echo [pypi]                                      >> ~/.pypirc
		echo username=${PYPI_USERNAME}                   >> ~/.pypirc
		echo password=${PYPI_PASSWORD}                   >> ~/.pypirc
		python -m twine upload --skip-existing "${WHEEL_FILENAME}"
	fi

	if [[ $(uname) == "Darwin" || "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]]; then
		echo "deploy to github releases"
		SDIST_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.tar.gz")
		response=$(curl -sH "Authorization: token ${GITHUB_API_TOKEN}" https://api.github.com/repos/sci-visus/OpenVisus/releases/tags/${TRAVIS_TAG})
		eval $(echo "$response" | grep -m 1 "id.:" | grep -w id | tr : = | tr -cd '[[:alnum:]]=')
		curl "$GITHUB_OAUTH_BASIC" --data-binary @"${SDIST_FILENAME}" -H "Authorization: token $GITHUB_API_TOKEN" -H "Content-Type: application/octet-stream" \
			"https://uploads.github.com/repos/sci-visus/OpenVisus/releases/$id/assets?name=$(basename ${SDIST_FILENAME})"
	fi

fi
 
# tests using CMake targets
if (( 1 == 1 )) ; then
	EchoSection "Test (1/2) OpenVisus"
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
	EchoSection "Test (2/2) OpenVisus"

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

echo "OpenVisus pyenv build finished"
exit 0



