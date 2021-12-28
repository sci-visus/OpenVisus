#!/bin/bash

set -e
set -x

source $(dirname "$0")/utils.sh

# install swig
curl -L --insecure https://cfhcable.dl.sourceforge.net/project/swig/swigwin/swigwin-4.0.2/swigwin-4.0.2.zip -O 
unzip swigwin-4.0.2.zip

# (DISABLED, are we still using ospray???)
# install ospray
# git clone https://github.com/sci-visus/ospray_win.git ExternalLibs/ospray_win

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`
PYTHON=${pythonLocation}/python
PIP_PLATFORM=win_amd64
BUILD_DIR=build

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR}
cmake -G "Visual Studio 16 2019" -A x64 -DQt5_DIR=${Qt5_Dir}/lib/cmake/Qt5 -DPython_EXECUTABLE=${PYTHON} -DSWIG_EXECUTABLE=../swigwin-4.0.2/swig.exe ../
cmake --build . --target ALL_BUILD --config Release --parallel 4
cmake --build . --target install	 --config Release            

CreateNonGuiVersion

pushd Release/OpenVisus
ConfigureAndTestCPython
DistribToPip
popd

pushd Release.nogui/OpenVisus
ConfigureAndTestCPython
DistribToPip
popd 

