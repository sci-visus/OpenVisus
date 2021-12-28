#!/bin/bash

set -e
set -x

source scripts/build_utils.sh

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# /////////////////////////////////////////////
# arguments

BUILD_DIR=${BUILD_DIR:-build_docker}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
PYTHON_VERSION=${PYTHON_VERSION:-3.8}
Qt5_DIR=${Qt5_DIR:-/opt/qt512}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_PASSWORD=${PYPI_PASSWORD:-}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}

ARCHITECTURE=`uname -m`
PIP_PLATFORM=unknown
if [[ "${ARCHITECTURE}" ==  "x86_64" ]] ; then PIP_PLATFORM=manylinux2010_${ARCHITECTURE} ; fi
if [[ "${ARCHITECTURE}" == "aarch64" ]] ; then PIP_PLATFORM=manylinux2014_${ARCHITECTURE} ; fi

# *** cpython ***
PYTHON=`which python${PYTHON_VERSION}`

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR}
cmake -DPython_EXECUTABLE=${PYTHON} -DQt5_DIR=${Qt5_DIR} -DVISUS_GUI=${VISUS_GUI} -DVISUS_MODVISUS=${VISUS_MODVISUS} -DVISUS_SLAM=${VISUS_SLAM} ../
make -j 
make install

pushd Release/OpenVisus
ConfigureAndTestCPython 
DistribToPip
popd

if [[ "${VISUS_GUI}" == "1" ]]; then
	CreateNonGuiVersion
	pushd Release.nogui/OpenVisus 
	ConfigureAndTestCPython 
	DistribToPip 
	popd
fi

# *** conda ***
InstallCondaUbuntu || true # can exist
ActivateConda
PYTHON=`which python`

# # fix for bdist_conda problem
cp -n \
	${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/distutils/command/bdist_conda.py \
	${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/site-packages/setuptools/_distutils/command/bdist_conda.py

pushd Release/OpenVisus 
ConfigureAndTestConda 
DistribToConda 
popd

if [[ "${VISUS_GUI}" == "1" ]]; then
	pushd Release.nogui/OpenVisus
	ConfigureAndTestConda
	DistribToConda
	popd
fi



