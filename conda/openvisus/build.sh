#!/bin/bash

set -ex

if [ $(uname) = "Darwin" ]; then
	OSX=1
else
   LINUX=1
fi

# source dir
SOURCE_DIR=$(pwd)

# build dir
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build/conda}

# change as needed
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

# I don't think I need openmp
DISABLE_OPENMP=${DISABLE_OPENMP:-1}

# todo: can I enable the Gui stuff?
VISUS_GUI=${VISUS_GUI:-0}

# all self contained
VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT:-1}

# having problems with some samples...
VISUS_TEST=${VISUS_TEST:0} 



rm -Rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# see https://www.anaconda.com/utilizing-the-new-compilers-in-anaconda-distribution-5/
if (( OSX==1 )); then

	if [ "${CONDA_BUILD_SYSROOT}" != "/opt/MacOSX10.9.sdk" ] ; then
		print "CONDA_BUILD_SYSROOT=${CONDA_BUILD_SYSROOT} is wrong. PLease fix your conda_build_config.yaml"
		exit -1
	fi

	CONDA_BUILD_SYSROOT=/opt/MacOSX10.9.sdk
	if [ ! -d ${CONDA_BUILD_SYSROOT} ] ; then
		print "Please install /opt/MacOSX10.9.sdk. From your shell:"
		print ""
		print "git clone https://github.com/phracker/MacOSX-SDKs"
		print "sudo mkdir -p /opt"
		print "sudo mv MacOSX-SDKs/MacOSX10.9.sdk /opt/"
		print "rm -Rf MacOSX-SDKs"
		exit -1
	fi
	
fi 

# scrgiorgio inspired by:
# https://github.com/conda-forge/libnetcdf-feedstock/tree/master/recipe
declare -a CMAKE_PLATFORM_FLAGS
if [[ ${c_compiler} != "toolchain_c" ]]; then
	export CC=$(basename ${CC})
	export CXX=$(basename ${CXX})
	if (( OSX == 1 )); then
		CMAKE_PLATFORM_FLAGS+=(-DCMAKE_OSX_SYSROOT="${CONDA_BUILD_SYSROOT}")
		export LDFLAGS=$(echo "${LDFLAGS}" | sed "s/-Wl,-dead_strip_dylibs//g")
	else
	  CMAKE_PLATFORM_FLAGS+=(-DCMAKE_TOOLCHAIN_FILE="${RECIPE_DIR}/cross-linux.cmake")
	fi
fi

# openvisus flags
CMAKE_PLATFORM_FLAGS+=(-DDISABLE_OPENMP=${DISABLE_OPENMP})
CMAKE_PLATFORM_FLAGS+=(-DVISUS_GUI=${VISUS_GUI})
CMAKE_PLATFORM_FLAGS+=(-DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT})
CMAKE_PLATFORM_FLAGS+=(-DPYTHON_VERSION=${PY_VER})
CMAKE_PLATFORM_FLAGS+=(-DPYTHON_EXECUTABLE=${PYTHON})
CMAKE_PLATFORM_FLAGS+=(-DVISUS_TEST=${VISUS_TEST}) 
CMAKE_PLATFORM_FLAGS+=(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}) 

cmake ${CMAKE_PLATFORM_FLAGS[@]} ${SOURCE_DIR}
cmake --build ./ --target all
cmake --build ./ --target install  
cmake --build ./ --target dist  

cd ${CMAKE_BUILD_TYPE}/site-packages/OpenVisus
$PYTHON setup.py install --single-version-externally-managed --record=record.txt

