#!/bin/bash

set -e
set -x

# ///////////////////////////////////////////////
function CreateNonGuiVersion() {
   mkdir -p Release.nogui
   cp -R   Release/OpenVisus Release.nogui/OpenVisus
   rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# ///////////////////////////////////////////////
function ConfigureAndTestCPython() {
   export PYTHONPATH=../
   $PYTHON   -m OpenVisus configure || true # this can fail on linux
   $PYTHON   -m OpenVisus test
   $PYTHON   -m OpenVisus test-gui || true # this can fail on linux
   unset PYTHONPATH
}

# ///////////////////////////////////////////////
function DistribToPip() {
   rm -Rf ./dist
   $PYTHON -m pip install setuptools wheel twine --upgrade 1>/dev/null || true
   $PYTHON setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=$PIP_PLATFORM
   if [[ "${GIT_TAG}" != "" ]] ; then
      $PYTHON -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing   "dist/*.whl" 
   fi
}

# install SDK
pushd /tmp 
rm -Rf MacOSX-SDKs 
git clone https://github.com/phracker/MacOSX-SDKs.git
popd

# install preconditions
brew install swig cmake

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`
PYTHON=${pythonLocation}/python
PIP_PLATFORM=macosx_10_9_x86_64
BUILD_DIR=build   

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR}

cmake \
	-GXcode \
	-DQt5_DIR=${Qt5_Dir}/lib/cmake/Qt5 \
	-DCMAKE_OSX_SYSROOT=/tmp/MacOSX-SDKs/MacOSX10.9.sdk \
	-DPython_EXECUTABLE=${PYTHON} \
	../

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