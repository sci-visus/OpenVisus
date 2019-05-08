#!/bin/bash

set -ex

if [ $(uname) = "Darwin" ]; then
	OSX=1
fi

# see https://www.anaconda.com/utilizing-the-new-compilers-in-anaconda-distribution-5/
if (( OSX == 1 )) ; then
	echo "CONDA_BUILD_SYSROOT=${CONDA_BUILD_SYSROOT}" 	
	if [[ ( -z "$CONDA_BUILD_SYSROOT") || ( ! -d "${CONDA_BUILD_SYSROOT}" ) ]] ; then
		echo "CONDA_BUILD_SYSROOT directory is wrong"
		exit -1
	fi
fi

if [[ ${c_compiler} != "toolchain_c" ]]; then
	export CC=$(basename ${CC})
	export CXX=$(basename ${CXX})

	if (( OSX == 1 )); then
		export CMAKE_OSX_SYSROOT="${CONDA_BUILD_SYSROOT}"
		export LDFLAGS=$(echo "${LDFLAGS}" | sed "s/-Wl,-dead_strip_dylibs//g")
	else
		export CMAKE_TOOLCHAIN_FILE="${RECIPE_DIR}/cross-linux.cmake"
	fi

fi

# NOTE environment variables are not passed to this script
# unless you add them to build/enviroment in meta.yml
export PYTHON_EXECUTABLE=${PYTHON}
export PYTHON_VERSION=${PY_VER}
USE_CONDA=1 INSIDE_CONDA=1 ./build.sh

# install into conda python
pushd build/RelWithDebInfo/site-packages/OpenVisus
${PYTHON} setup.py install --single-version-externally-managed --record=record.txt
popd



