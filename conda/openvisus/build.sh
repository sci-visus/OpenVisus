#!/bin/bash

set -ex

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-$SOURCE_DIR/build/conda}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

if [ $(uname) = "Darwin" ]; then
	OSX=1
fi

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
CMAKE_PLATFORM_FLAGS+=(-DDISABLE_OPENMP=1)
CMAKE_PLATFORM_FLAGS+=(-DVISUS_GUI=0)
CMAKE_PLATFORM_FLAGS+=(-DVISUS_INTERNAL_DEFAULT=1)
CMAKE_PLATFORM_FLAGS+=(-DPYTHON_VERSION=${PY_VER})
CMAKE_PLATFORM_FLAGS+=(-DPYTHON_EXECUTABLE=${PYTHON})
CMAKE_PLATFORM_FLAGS+=(-DVISUS_TEST=0) # having problems with some samples...

cmake \
	${CMAKE_PLATFORM_FLAGS[@]} \
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
	${SOURCE_DIR}

cmake --build ./ --target all -- -j 4

cmake --build ./ --target install  

pushd install
$PYTHON setup.py install --single-version-externally-managed --record=record.txt
popd

set +ex
