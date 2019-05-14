#!/bin/bash

set -ex

if [ $(uname) = "Darwin" ]; then
	OSX=1
fi

# NOTE environment variables are not passed to this script
# unless you add them to build/enviroment in meta.yml
SOURCE_DIR=$(pwd)
BUILD_DIR=${SOURCE_DIR}/build_conda
CMAKE_BUILD_TYPE=RelWithDebInfo
VISUS_OPENMP=0
VISUS_MODVISUS=1
VISUS_GUI=0 # todo: can Qt5 work?

# ////////////////////////////////////////////////////////////////////////
function NeedApache {

	APACHE_DIR=${BUILD_DIR}/.apache

	if [ ! -f "${APACHE_DIR}/include/httpd.h" ] ; then

		mkdir -p ${APACHE_DIR} 

		curl -fsSL --insecure "https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-2.2.6.tar.bz2" | tar xj
		pushd expat-2.2.6
		./configure --prefix=${APACHE_DIR} 
		make -s 
		make install 
		popd
		
		curl -fsSL --insecure "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" | tar xz
		pushd pcre-8.42 
		./configure --prefix=${APACHE_DIR}  
		make -s   
		make install  
		popd	

		curl -fsSL --insecure "http://it.apache.contactlab.it/httpd/httpd-2.4.38.tar.gz" | tar xz 
		pushd httpd-2.4.38		
		curl -fsSL --insecure "http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz"      | tar xz
		curl -fsSL --insecure "http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz" | tar xz	
		mv ./apr-1.6.5      ./srclib/apr
		mv ./apr-util-1.6.1 ./srclib/apr-util
		./configure --prefix=${APACHE_DIR} --with-included-apr --with-pcre=${APACHE_DIR} --with-ssl=${APACHE_DIR} --with-expat=${APACHE_DIR}  
		make -s  
		make install  
		popd
	fi
}

if (( OSX == 1 )) ; then
	VISUS_MODVISUS=0
fi

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

declare -a cmake_opts
cmake_opts+=(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
cmake_opts+=(-DVISUS_OPENMP=${VISUS_OPENMP})

# setup compiler
if [[ ${c_compiler} != "toolchain_c" ]]; then

	export CC=$(basename ${CC})
	export CXX=$(basename ${CXX})

	if (( OSX == 1 )); then
		export LDFLAGS=$(echo "${LDFLAGS}" | sed "s/-Wl,-dead_strip_dylibs//g")
	 	#see https://www.anaconda.com/utilizing-the-new-compilers-in-anaconda-distribution-5/
		echo "CONDA_BUILD_SYSROOT=${CONDA_BUILD_SYSROOT}" 	
		if [[ ( -z "$CONDA_BUILD_SYSROOT" ) || ( ! -d "${CONDA_BUILD_SYSROOT}" ) ]] ; then
			echo "CONDA_BUILD_SYSROOT directory is wrong"
			exit -1
		fi
		cmake_opts+=(-DCMAKE_OSX_SYSROOT=${CONDA_BUILD_SYSROOT})
	else
		cmake_opts+=(-DCMAKE_TOOLCHAIN_FILE="${RECIPE_DIR}/cross-linux.cmake")
	fi
fi

# apache
cmake_opts+=(-DVISUS_MODVISUS="${VISUS_MODVISUS}")
if (( VISUS_MODVISUS == 1 )); then
	NeedApache	
	cmake_opts+=(-DAPACHE_DIR="${APACHE_DIR}")
	cmake_opts+=(-DAPR_DIR="${APACHE_DIR}")
fi

# qt5 (can it work?)
cmake_opts+=(-DVISUS_GUI="${VISUS_GUI}") 
if (( VISUS_GUI == 1 )) ; then
	echo "todo..."
	exit -1
fi

cmake ${cmake_opts[@]} ${SOURCE_DIR}
cmake --build ./ --target "all"   --config ${CMAKE_BUILD_TYPE}
cmake --build .  --target install --config ${CMAKE_BUILD_TYPE}

# install into conda python
pushd ${CMAKE_BUILD_TYPE}/OpenVisus
${PYTHON} setup.py install --single-version-externally-managed --record=record.txt
popd




