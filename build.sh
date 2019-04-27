#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}
CACHE_DIR=${CACHE_DIR:-${BUILD_DIR}/.cache}
PYENV_ROOT=${PYENV_ROOT:-${HOME}/.pyenv}

# cmake flags
PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}
DISABLE_OPENMP=${DISABLE_OPENMP:-0}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

# conda stuff
USE_CONDA=${USE_CONDA:-0}
DEPLOY_CONDA=${DEPLOY_CONDA:-0}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}

# deploy pypi
DEPLOY_PYPI=${DEPLOY_PYPI:-0}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_PASSWORD=${PYPI_PASSWORD:-}

# deploy github
DEPLOY_GITHUB=${DEPLOY_GITHUB:-0}
GITHUB_API_TOKEN=${GITHUB_API_TOKEN:-}
TRAVIS_TAG=${TRAVIS_TAG:-}

# in case you want to try manylinux-like compilation (for debugging only)
UseInstalledPackages=${UseInstalledPackages:-1}

PYTHON_MAJOR_VERSION=${PYTHON_VERSION:0:1}
PYTHON_MINOR_VERSION=${PYTHON_VERSION:2:1}	

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
function BeginSection {
	set +x	
	echo "//////////////////////////////////////////////////////////////////////// $1"
	set -x
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function Preamble {	

	
	BeginSection "Preamble"

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
		else

		  export DEPLOY_GITHUB=1

		  if [[ "${TRAVIS_TAG}" != "" ]]; then

			  if [[ "$TRAVIS_OS_NAME" == "osx"  ]]; then
				  export DEPLOY_PYPI=1
			  fi    
		
			  if [[ "$TRAVIS_OS_NAME" == "linux" && "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]]; then
				  export DEPLOY_PYPI=1
			  fi    
 
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
		CENTOS_VERSION=${__version__}
		CENTOS_MAJOR=${__major__}
		echo "Detected centos ${CENTOS_VERSION}"

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
function BuildUsingDocker {

	# note: sudo is needed anyway otherwise travis fails
	sudo docker rm -f mydocker 2>/dev/null || true

	declare -a docker_opts

	docker_opts+=(-v ${SOURCE_DIR}:/root/OpenVisus)
	docker_opts+=(-e SOURCE_DIR=/root/OpenVisus)

	docker_opts+=(-v ${BUILD_DIR}:/root/OpenVisus.build)
	docker_opts+=(-e BUILD_DIR=/root/OpenVisus.build)

	docker_opts+=(-v ${CACHE_DIR}:/root/OpenVisus.cache)
	docker_opts+=(-e CACHE_DIR=/root/OpenVisus.cache)

	#docker_opts+=(-v ${PYENV_ROOT}:/root/.pyenv)
	#docker_opts+=(-e ${PYENV_ROOT}=/root/.pyenv)

	docker_opts+=(-e PYTHON_VERSION=${PYTHON_VERSION})
	docker_opts+=(-e DISABLE_OPENMP=${DISABLE_OPENMP})
	docker_opts+=(-e VISUS_GUI=${VISUS_GUI})
	docker_opts+=(-e VISUS_MODVISUS=${VISUS_MODVISUS})
	docker_opts+=(-e CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

	# deploy to conda
	docker_opts+=(-e USE_CONDA=${USE_CONDA})
	docker_opts+=(-e DEPLOY_CONDA=${DEPLOY_CONDA})
	docker_opts+=(-e ANACONDA_TOKEN=${ANACONDA_TOKEN})

	# deploy to pypi
	docker_opts+=(-e DEPLOY_PYPI=${DEPLOY_PYPI})
	docker_opts+=(-e PYPI_USERNAME=${PYPI_USERNAME})
	docker_opts+=(-e PYPI_PASSWORD=${PYPI_PASSWORD})

	# deploy to github
	docker_opts+=(-e DEPLOY_GITHUB=${DEPLOY_GITHUB})
	docker_opts+=(-e GITHUB_API_TOKEN=${GITHUB_API_TOKEN})
	docker_opts+=(-e TRAVIS_TAG=${TRAVIS_TAG})

	sudo docker run -d -ti --name mydocker ${docker_opts[@]} ${DOCKER_IMAGE} /bin/bash
	sudo docker exec mydocker /bin/bash -c "cd /root/OpenVisus && ./build.sh"

	sudo chown -R "$USER":"$USER" ${BUILD_DIR} 1>/dev/null && :
	sudo chmod -R u+rwx           ${BUILD_DIR} 1>/dev/null && :
}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function InstallMiniconda {
	
	pushd $HOME
	if (( OSX == 1 )) ; then
		DownloadFile https://repo.continuum.io/miniconda/Miniconda${PYTHON_MAJOR_VERSION}-latest-MacOSX-x86_64.sh
		bash Miniconda${PYTHON_MAJOR_VERSION}-latest-MacOSX-x86_64.sh -b
	else
		DownloadFile https://repo.continuum.io/miniconda/Miniconda${PYTHON_MAJOR_VERSION}-latest-Linux-x86_64.sh
		bash Miniconda${PYTHON_MAJOR_VERSION}-latest-Linux-x86_64.sh -b
	fi
	popd
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
function InstallPrerequisites {

	BeginSection "InstallPrerequisites"

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

		InstallPackages git curl ca-certificates uuid-dev automake bzip2 libffi-dev build-essential make
		InstallPatchElf

	elif (( OPENSUSE == 1 )) ; then

		if (( IsRoot == 1 )) ; then
			${SudoCmd} zypper --non-interactive update 1>/dev/null  && :
			${SudoCmd} zypper --non-interactive install --type pattern devel_basis
		fi

		InstallPackages git curl lsb-release libuuid-devel libffi-devel gcc-c++ make
		InstallPatchElf
		
	elif (( CENTOS == 1 )) ; then

		if (( IsRoot == 1 )) ; then
			${SudoCmd} yum update 1>/dev/null  && :
		fi

		InstallPackages zlib zlib-devel curl libffi-devel gcc-c++ make
		InstallPatchElf

		if (( VISUS_GUI ==  1 )); then
			InstallPackages mesa-libGL-devel mesa-libGLU-devel 
		fi

	fi

	echo "Installed prerequisites"
	return 0
}




# //////////////////////////////////////////////////////
function InstallCMake {

	BeginSection "InstallCMake"

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

	BeginSection "InstallSwig"
	
	unset SWIG_EXECUTABLE

	if [ -f "${CACHE_DIR}/bin/swig" ]; then	
		echo "Using cached swig"
		SWIG_EXECUTABLE=${CACHE_DIR}/bin/swig
		return 0
	fi

	if (( OSX == 1 || UseInstalledPackages == 1 )); then

		InstallPackages swig && :

		# already installed
		if [[ -x "$(command -v swig)" ]]; then
			GetVersionFromCommand "swig -version" "SWIG Version "
			if (( __major__>= 3)); then
				echo "Good version: swig==${__version__}"
				SWIG_EXECUTABLE=$(which swig)	
				return 0
			else
				echo "Wrong version: swig==${__version__}"
			fi
		fi 

		InstallPackages swig3.0 && :

		# already installed
		if [[ -x "$(command -v swig3.0)" ]]; then
			SWIG_EXECUTABLE=$(which swig3.0)
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
	./configure --prefix=${CACHE_DIR} 1>/dev/null 
	make -s -j 4 1>/dev/null 
	make install 1>/dev/null 
	popd
	rm -Rf swig-3.0.12

	SWIG_EXECUTABLE=${CACHE_DIR}/bin/swig
	return 0
}

# //////////////////////////////////////////////////////
function InstallPatchElf {

	BeginSection "InstallPatchElf"

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
	./configure --prefix=${CACHE_DIR} 1>/dev/null 
	make -s 1>/dev/null 
	make install 1>/dev/null 
	autoreconf -f -i
	./configure --prefix=${CACHE_DIR} 1>/dev/null 
	make -s 1>/dev/null 
	make install 1>/dev/null 
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
	./config --prefix=${CACHE_DIR} -fPIC shared 1>/dev/null 
	make -s 1>/dev/null  
	make install 1>/dev/null 
	popd
	rm -Rf openssl-1.0.2a
	
	export OPENSSL_DIR="${CACHE_DIR}" 
	export LD_LIBRARY_PATH="${CACHE_DIR}/lib:${LD_LIBRARY_PATH}"
	return 0
}


# //////////////////////////////////////////////////////
function InstallApache {

	unset APR_DIR
	unset APACHE_DIR

	BeginSection "InstallApache"

	if [ -f "${CACHE_DIR}/include/httpd.h" ] ; then
		echo "Using cached apache"
		APR_DIR={CACHE_DIR}
		APACHE_DIR=${CACHE_DIR}
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
	./configure --prefix=${CACHE_DIR} 1>/dev/null 
	make -s 1>/dev/null 
	make install 1>/dev/null 
	popd
	rm -Rf apr-1.6.5

	# install apr utils 
	url="http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd apr-util-1.6.1
	./configure --prefix=${CACHE_DIR} --with-apr=${CACHE_DIR} 1>/dev/null  
	make -s 1>/dev/null 
	make install 1>/dev/null 
	popd
	rm -Rf apr-util-1.6.1

	# install pcre 
	url="https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd pcre-8.42
	./configure --prefix=${CACHE_DIR} 1>/dev/null 
	make -s 1>/dev/null 
	make install 1>/dev/null 
	popd
	rm -Rf pcre-8.42

	# install httpd
	url="http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz"
	filename=$(basename ${url})
	DownloadFile ${url}
	tar xzf ${filename}
	pushd httpd-2.4.38
	./configure --prefix=${CACHE_DIR} --with-apr=${CACHE_DIR} --with-pcre=${CACHE_DIR} --with-ssl=${CACHE_DIR} 1>/dev/null 
	make -s 1>/dev/null 
	make install 1>/dev/null 
	popd
	rm -Rf httpd-2.4.38
	
	APR_DIR={CACHE_DIR}
	APACHE_DIR=${CACHE_DIR}
	return 0
}



# //////////////////////////////////////////////////////
function InstallQt5 {
	
	BeginSection "InstallQt5"

	# already set by user
	if [[ "${Qt5_DIR}" != "" ]] ; then
		return 0
	fi
	
	unset Qt5_DIR

	# install qt 5.11 (instead of 5.12 which is not supported by PyQt5)
	if (( OSX == 1 )); then

		if [ ! -d /usr/local/Cellar/qt/5.11.2_1 ] ; then
			echo "installing brew Qt5"
			brew uninstall qt5 1>/dev/null 2>/dev/null && :
			InstallPackages "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"
		fi

		Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5
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
				return 0
			fi
		fi

		# opensuse
		if (( OPENSUSE == 1 )) ; then
			InstallPackages glu-devel libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel && : 
			if [ $? == 0 ] ; then return 0 ; fi
		fi

	fi

	# backup plan , use a minimal Qt5 which does not need SUDO
	echo "Using minimal Qt5"
	QT_VERSION=5.11.2
	Qt5_DIR=${CACHE_DIR}/qt${QT_VERSION}/lib/cmake/Qt5

	# if you want to create a "new" minimal Qt5 see CMake/Dockerfile.BuildQt5
	# note this is only to allow compilation
	# in order to execute it you need to use PyUseQt 
	if [ ! -d "${Qt5_DIR}" ] ; then
		url="http://atlantis.sci.utah.edu/qt/qt${QT_VERSION}.tar.gz"
		filename=$(basename ${url})
		DownloadFile "${url}"
		tar xzf ${filename} -C ${CACHE_DIR} 
	fi

	return 0
}


# ///////////////////////////////////////////////////////
function InstallPython {

	BeginSection InstallPython
	
	if (( PYTHON_MAJOR_VERSION > 2 )) ; then 
		PYTHON_M_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}m 
	else
		PYTHON_M_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
	fi	
	
	# install python using pyenv
	if (( OSX == 1 )) ; then
		
		# pyenv does not support 3.7.x  maxosx 10.(12|13)
		if (( PYTHON_MAJOR_VERSION > 2 )); then
			PYTHON_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
			package_name=python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}
			brew install sashkab/python/${package_name}
			package_dir=$(brew --prefix ${package_name})
			PYTHON_EXECUTABLE=${package_dir}/bin/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
			PYTHON_INCLUDE_DIR=${package_dir}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/include/python${PYTHON_M_VERSION}
			PYTHON_LIBRARY=${package_dir}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/lib/libpython${PYTHON_M_VERSION}.dylib
			
		else

			InstallPackages readline zlib openssl openssl@1.1 pyenv && :
	
			# activate pyenv
			eval "$(pyenv init -)"
	
			export CONFIGURE_OPTS="--enable-shared"
			export CFLAGS=" -I$(brew --prefix readline)/include -I$(brew --prefix zlib)/include"
			export LDFLAGS="-L$(brew --prefix readline)/lib     -L$(brew --prefix zlib)/lib"
			export CPPFLAGS="${CFLAGS}"
	
			pyenv install --skip-existing ${PYTHON_VERSION} && :
			if [ $? != 0 ] ; then 
				echo "pyenv failed to install"
				pyenv install --list
				exit -1
			fi
	
			unset CONFIGURE_OPTS
			unset CFLAGS
			unset LDFLAGS
			unset CPPFLAGS
			
			eval "$(pyenv init -)"
			pyenv global ${PYTHON_VERSION}
			pyenv rehash
		
			PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
			PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION}
			PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.dylib 	
			
		fi		
		
	else

		InstallOpenSSL

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
		
		# activate pyenv
		export PATH="$HOME/.pyenv/bin:$PATH"
		eval "$(pyenv init -)"
		pyenv global ${PYTHON_VERSION}
		pyenv rehash
	
		PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
		PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION}
		PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
		
	fi
	
	# install python packages
	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine auditwheel		
	
	return 0
}

# /////////////////////////////////////////////////////////////////////
function DeployToPyPi {


	BeginSection "DeployToPyPi"

	WHEEL_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.whl")
	echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
	echo [distutils]                                  > ~/.pypirc
	echo index-servers =  pypi                       >> ~/.pypirc
	echo [pypi]                                      >> ~/.pypirc
	echo username=${PYPI_USERNAME}                   >> ~/.pypirc
	echo password=${PYPI_PASSWORD}                   >> ~/.pypirc

	${PYTHON_EXECUTABLE} -m twine upload --skip-existing "${WHEEL_FILENAME}"
}


# /////////////////////////////////////////////////////////////////////
function DeployToGitHub {
	
	BeginSection "DeployToGitHub"

	filename=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages/OpenVisus/dist -iname "*.tar.gz")

	# rename to avoid collisions
	if (( UBUNTU == 1 )); then
		old_filename=$filename
		filename=${filename/manylinux1_x86_64.tar.gz/ubuntu.${UBUNTU_VERSION}.tar.gz}
		mv $old_filename $filename

	elif (( OPENSUSE == 1 )); then
		old_filename=$filename
		filename=${filename/manylinux1_x86_64.tar.gz/opensuse.tar.gz}
		mv $old_filename $filename

	elif (( CENTOS == 1 )); then
		old_filename=$filename
		filename=${filename/manylinux1_x86_64.tar.gz/centos.${CENTOS_VERSION}.tar.gz}
		mv $old_filename $filename

	fi

	response=$(curl -sH "Authorization: token ${GITHUB_API_TOKEN}" https://api.github.com/repos/sci-visus/OpenVisus/releases/tags/${TRAVIS_TAG})
	eval $(echo "$response" | grep -m 1 "id.:" | grep -w id | tr : = | tr -cd '[[:alnum:]]=')

	curl --data-binary @"${filename}" \
		-H "Authorization: token $GITHUB_API_TOKEN" \
		-H "Content-Type: application/octet-stream" \
		"https://uploads.github.com/repos/sci-visus/OpenVisus/releases/$id/assets?name=$(basename ${filename})"
}

# /////////////////////////////////////////////////////////////////////
function DeployToConda {

	BeginSection "Deploy conda"
	CONDA_BUILD_FILENAME=$(find ${HOME}/miniconda${PYTHON_MAJOR_VERSION}/conda-bld -iname "openvisus*.tar.bz2")
	echo "Doing deploy to anaconda ${CONDA_BUILD_FILENAME}..."
	anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
}


# /////////////////////////////////////////////////////////////////////
Preamble

mkdir -p ${BUILD_DIR}
mkdir -p ${CACHE_DIR}

if [[ "$DOCKER_IMAGE" != "" ]] ; then
	BeginSection "BuildUsingDocker"
	BuildUsingDocker
	echo "BuildUsingDocker done"
	exit 0
fi

pushd ${BUILD_DIR}
export PATH=${CACHE_DIR}/bin:$PATH

InstallPrerequisites && :

if (( USE_CONDA == 1 )) ; then

	# redirecting to conda/OpenVisus/build.sh
	BeginSection "Build OpenVisus using conda"

	# here I need sudo! 
	if (( OSX ==  1)) ; then
		if [ ! -d /opt/MacOSX10.9.sdk ] ; then
		  if (( IsRoot == 1 )) ; then
				git clone  https://github.com/phracker/MacOSX-SDKs.git 
				sudo mv MacOSX-SDKs/MacOSX10.9.sdk /opt/
				rm -Rf MacOSX-SDKs
			else
				echo "Missing /opt/MacOSX10.9.sdk, but to install it I need sudo"
				exit -1
			fi
		fi
	fi

	# install Miniconda
	MINICONDA_ROOT=$HOME/miniconda${PYTHON_MAJOR_VERSION}
	if [ ! -d  ${MINICONDA_ROOT} ]; then
		InstallMiniconda
	fi

	# config Miniconda
	export PATH="${MINICONDA_ROOT}/bin:$PATH"
	
	hash -r	
	conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
	conda install -q conda-build anaconda-client && :
	conda update  -q conda conda-build           && :
	conda install -q python=${PYTHON_VERSION}	   && :
	
	PYTHON_EXECUTABLE=python

	# build Openvisus (see conda/OpenVisus/* files)
	pushd ${SOURCE_DIR}/conda
	conda-build -q openvisus
	conda install -q --use-local openvisus
	popd
	
else	

	InstallCMake
	InstallSwig
	InstallPython
	
	if (( OSX != 1 && VISUS_MODVISUS == 1 )); then	
		InstallApache
	fi
	
	if (( VISUS_GUI == 1 )); then
		InstallQt5
	fi
	
	BeginSection "Build OpenVisus using pyenv"
	
	declare -a cmake_opts
	
	if (( OSX == 1 )) ; then
		cmake_opts+=(-GXcode)
		CMAKE_TEST_STEP="RUN_TESTS"
		CMAKE_ALL_STEP="ALL_BUILD"
	else
		CMAKE_TEST_STEP="test"
		CMAKE_ALL_STEP="all"
	fi
	
	cmake_opts+=(-DDISABLE_OPENMP=${DISABLE_OPENMP})
	cmake_opts+=(-DVISUS_GUI=${VISUS_GUI})
	cmake_opts+=(-DVISUS_MODVISUS=${VISUS_MODVISUS})
	cmake_opts+=(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
	
	cmake_opts+=(-DPYTHON_VERSION=${PYTHON_VERSION})	
	cmake_opts+=(-DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE})
	cmake_opts+=(-DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR})
	cmake_opts+=(-DPYTHON_LIBRARY=${PYTHON_LIBRARY})
	
	cmake_opts+=(-DSWIG_EXECUTABLE=${SWIG_EXECUTABLE})
	
	if [[ "${APR_DIR}" != "" ]]; then
		cmake_opts+=(-DAPR_DIR=${APR_DIR})
	fi
	
	if [[ "${APACHE_DIR}" != "" ]]; then
		cmake_opts+=(-DAPACHE_DIR=${APACHE_DIR})
	fi
	
	if [[ "${Qt5_DIR}" != "" ]]; then
		cmake_opts+=(-DQt5_DIR=${Qt5_DIR})
	fi
	
	cmake ${cmake_opts[@]} ${SOURCE_DIR}
	
	if (( TRAVIS == 1 && OSX == 1 )) ; then
		cmake --build ./ --target ${CMAKE_ALL_STEP} --config ${CMAKE_BUILD_TYPE} | xcpretty -c
	else
		cmake --build ./ --target ${CMAKE_ALL_STEP} --config ${CMAKE_BUILD_TYPE}
	fi	
	
	# install step
	BeginSection "Install OpenVisus"
	cmake --build . --target install --config ${CMAKE_BUILD_TYPE}

	# dist test	
	if (( DEPLOY_GITHUB == 1 || DEPLOY_PYPI == 1 )) ; then
		BeginSection "OpenVisus cmake dist"
		cmake --build . --target dist --config ${CMAKE_BUILD_TYPE}
	fi	
	
	# tests step
	if (( 1 == 1 )) ; then
		BeginSection "Test OpenVisus (cmake ${CMAKE_TEST_STEP})"
		cmake --build  ./ --target  ${CMAKE_TEST_STEP} --config ${CMAKE_BUILD_TYPE}	
	fi
	
	# external app step
	if (( 1 == 1 )) ; then
		BeginSection "Test OpenVisus (cmake external app)"
		cmake --build ./ --target  simple_query --config ${CMAKE_BUILD_TYPE}
		if (( VISUS_GUI == 1 )) ; then
			cmake --build   . --target   simple_viewer2d --config ${CMAKE_BUILD_TYPE}
		fi	
	fi
	
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}/site-packages
	
fi

popd

# test extending python
if (( 1 == 1 )); then
	BeginSection "Test OpenVisus (extending python)"
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	${PYTHON_EXECUTABLE} Samples/python/Array.py
	${PYTHON_EXECUTABLE} Samples/python/Dataflow.py
	${PYTHON_EXECUTABLE} Samples/python/Idx.py
	popd
fi

# test stand alone scripts
if (( USE_CONDA == 0 )) ; then
	BeginSection "Test OpenVisus (embedding python)"
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	if (( OSX == 1 )) ; then
		./visus.command
	else
		./visus.sh
	fi
	popd
fi

if (( DEPLOY_PYPI == 1 )) ; then 
	DeployToPyPi   
fi

if (( DEPLOY_GITHUB == 1 )) ; then 
	DeployToGitHub 
fi

# deploy to conda 
if (( DEPLOY_CONDA == 1 )) ; then
	DeployToConda
fi

echo "OpenVisus build finished"
exit 0



