#!/bin/bash

set -ex

BUILD_DIR=${BUILD_DIR:-build_docker}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
Python_EXECUTABLE=${Python_EXECUTABLE:-}
Qt5_DIR=${Qt5_DIR:-/opt/qt515}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_TOKEN=${PYPI_TOKEN:-}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}
GIT_TAG=${GIT_TAG:-}

# ///////////////////////////////////////////////
function CreateNonGuiVersion() {
   mkdir -p Release.nogui
   cp -R   Release/OpenVisus Release.nogui/OpenVisus
   rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# ///////////////////////////////////////////////
function ConfigureAndTestCPython() {
   export PYTHONPATH=../
   ${Python_EXECUTABLE} -m OpenVisus configure || true # this can fail on linux
   ${Python_EXECUTABLE} -m OpenVisus test
   ${Python_EXECUTABLE} -m OpenVisus test-gui || true # this can fail on linux
   unset PYTHONPATH
}

# ///////////////////////////////////////////////
function DistribToPip() {
  rm -Rf ./dist
  
  # this fails a LOT on linux, and this is a good combination for OLD manyliux
  ${Python_EXECUTABLE} -m pip install --upgrade pip         ||  true 
  $${Python_EXECUTABLE} -m pip install setuptools==59.6.0 wheel==0.37.0 cryptography==3.4.0 twine==3.8.0 readme-renderer==34.0 ||  true

  
  ${Python_EXECUTABLE}  setup.py -q bdist_wheel --python-tag=$PYTHON_TAG --plat-name=$PIP_PLATFORM
  
  if [[ "${GIT_TAG}" != "" ]] ; then
    ${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing   "dist/*.whl" 
  fi
}

# this is for linux/docker (is this needed?)
yum install -y libffi-devel

# make sure pip is updated
${Python_EXECUTABLE} -m pip install --upgrade pip || true

# detect architecture
if [[ "1" == "1" ]]; then
  ARCHITECTURE=`uname -m`
  PIP_PLATFORM=unknown
  if [[ "${ARCHITECTURE}" ==  "x86_64" ]] ; then PIP_PLATFORM=manylinux2010_${ARCHITECTURE} ; fi
  if [[ "${ARCHITECTURE}" == "aarch64" ]] ; then PIP_PLATFORM=manylinux2014_${ARCHITECTURE} ; fi
fi

# compile OpenVisus
if [[ "1" == "1" ]]; then
  mkdir -p ${BUILD_DIR} 
  cd ${BUILD_DIR}
  cmake \
    -DPython_EXECUTABLE=${Python_EXECUTABLE}  \
    -DQt5_DIR=${Qt5_DIR} \
    -DVISUS_GUI=${VISUS_GUI} \
    -DVISUS_MODVISUS=${VISUS_MODVISUS} \
    -DVISUS_SLAM=${VISUS_SLAM} \
    ../
  make -j
  make install
fi

if [[ "1" == "1" ]]; then
  pushd Release/OpenVisus
  ConfigureAndTestCPython 
  DistribToPip
  popd
fi

if [[ "${VISUS_GUI}" == "1" ]]; then
  CreateNonGuiVersion
  pushd Release.nogui/OpenVisus 
  ConfigureAndTestCPython 
  DistribToPip 
  popd
fi

# scrgiorgio: disabled conda on linux for now, getting `ImportError: /lib64/libc.so.6: version `GLIBC_2.14' not found1

echo "All done ubuntu"





