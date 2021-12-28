#!/bin/bash

set -e
set -x

source scripts/utils.sh

# install swig
curl -L --insecure https://cfhcable.dl.sourceforge.net/project/swig/swigwin/swigwin-4.0.2/swigwin-4.0.2.zip -O 
unzip swigwin-4.0.2.zip

# install ospray
git clone https://github.com/sci-visus/ospray_win.git ExternalLibs/ospray_win

# configure conda
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no             1>/dev/null
conda install -c conda-forge -y conda cmake pyqt=5.12  anaconda-client  conda-build wheel 1>/dev/null

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`
PYTHON=`which python`
BUILD_DIR=build        

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR}
cmake -G "Visual Studio 16 2019" -A x64 -DQt5_DIR=${CONDA_PREFIX}/Library/lib/cmake/Qt5 -DPython_EXECUTABLE=${PYTHON} -DSWIG_EXECUTABLE=../swigwin-4.0.2/swig.exe ../
cmake --build . --target ALL_BUILD --config Release --parallel 4
cmake --build . --target install	 --config Release

CreateNonGuiVersion

pushd Release/OpenVisus
ConfigureAndTestConda

find CONDA_PREFIX/Lib
cp -n $CONDA_PREFIX/Lib/distutils/command/bdist_conda.py $CONDA_PREFIX/Lib/site-packages/setuptools/bdist_conda.py
DistribToConda
popd

pushd Release.nogui/OpenVisus
ConfigureAndTestConda
cp -n $CONDA_PREFIX/Lib/distutils/command/bdist_conda.py $CONDA_PREFIX/Lib/site-packages/setuptools/bdist_conda.py
DistribToConda
popd

