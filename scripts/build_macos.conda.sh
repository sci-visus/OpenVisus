#!/bin/bash

set -e
set -x 

source scripts/utils.sh 

# configure conda
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no   1>/dev/null
conda install --yes -c conda-forge conda anaconda-client conda-build wheel pyqt=5.12  1>/dev/null

# for for `bdist_conda` problem
cp -n \
  ${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/distutils/command/bdist_conda.py \
  ${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/site-packages/setuptools/_distutils/command/bdist_conda.py              

# install SDK
pushd /tmp 
rm -Rf MacOSX-SDKs 
git clone https://github.com/phracker/MacOSX-SDKs.git
popd

# install preconditions
brew install swig cmake

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`
PYTHON=`which python`
BUILD_DIR=build

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR}
cmake  -GXcode -DQt5_DIR=${CONDA_PREFIX}/lib/cmake/Qt5 -DCMAKE_OSX_SYSROOT=/tmp/MacOSX-SDKs/MacOSX10.9.sdk -DPython_EXECUTABLE=${PYTHON} ../
cmake --build . --target ALL_BUILD --config Release --parallel 4
cmake --build . --target install	 --config Release     

CreateNonGuiVersion

pushd Release/OpenVisus
ConfigureAndTestConda  
DistribToConda  
popd

pushd Release.nogui/OpenVisus
ConfigureAndTestConda
DistribToConda
popd
