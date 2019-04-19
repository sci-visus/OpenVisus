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

# I don't think I need to need mod visus
VISUS_MODVISUS=${VISUS_MODVISUS:-0}


rm -Rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# see https://www.anaconda.com/utilizing-the-new-compilers-in-anaconda-distribution-5/
if (( OSX == 1 )) ; then
	echo "CONDA_BUILD_SYSROOT=${CONDA_BUILD_SYSROOT}" 	
	if [[ ( -z "$CONDA_BUILD_SYSROOT") || ( ! -d "${CONDA_BUILD_SYSROOT}" ) ]] ; then
		echo "CONDA_BUILD_SYSROOT directory is wrong"
		exit -1
	fi
fi

# inspired by: https://github.com/conda-forge/libnetcdf-feedstock/tree/master/recipe
declare -a cmake_opts

if [[ ${c_compiler} != "toolchain_c" ]]; then
	export CC=$(basename ${CC})
	export CXX=$(basename ${CXX})
	if (( OSX == 1 )); then
		cmake_opts+=(-DCMAKE_OSX_SYSROOT="${CONDA_BUILD_SYSROOT}")
		export LDFLAGS=$(echo "${LDFLAGS}" | sed "s/-Wl,-dead_strip_dylibs//g")
	else
	  cmake_opts+=(-DCMAKE_TOOLCHAIN_FILE="${RECIPE_DIR}/cross-linux.cmake")
	fi
fi

# openvisus flags
cmake_opts+=(-DDISABLE_OPENMP=${DISABLE_OPENMP})
cmake_opts+=(-DVISUS_GUI=${VISUS_GUI})
cmake_opts+=(-DPYTHON_VERSION=${PY_VER})
cmake_opts+=(-DPYTHON_EXECUTABLE=${PYTHON})
cmake_opts+=(-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}) 

cmake ${cmake_opts[@]} ${SOURCE_DIR}

cmake --build ./ --target all     --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target install --config ${CMAKE_BUILD_TYPE}
cmake --build ./ --target dist    --config ${CMAKE_BUILD_TYPE}

cd ${CMAKE_BUILD_TYPE}/site-packages/OpenVisus

$PYTHON setup.py install --single-version-externally-managed --record=record.txt

