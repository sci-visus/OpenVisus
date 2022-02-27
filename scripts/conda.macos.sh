#!/bin/bash

set -ex

PYTHON_VERSION=${PYTHON_VERSION:-3.8}
PYQT_VERSION=${PYQT_VERSION:-5.12}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-0}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# //////////////////////////////////////////////////////////////
function InstallConda() {
   if [[ ! -d "~/miniforge3" ]]; then 
      pushd ~
      curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-x86_64.sh
      bash Miniforge3-MacOSX-x86_64.sh -b # silent
      rm -f Miniforge3-MacOSX-x86_64.sh
      popd
   fi
}

# //////////////////////////////////////////////////////////////
function ActivateConda() {
   source ~/miniforge3/etc/profile.d/conda.sh || true # can be already activated
   conda config --set always_yes yes --set anaconda_upload no
   conda create --name my-env -c conda-forge python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel pyqt=$PYQT_VERSION swig cmake
   conda activate my-env
}

# //////////////////////////////////////////////////////////////
function InstallSDK() {
  pushd /tmp 
  rm -Rf MacOSX-SDKs 
  git clone https://github.com/phracker/MacOSX-SDKs.git  --quiet
  popd 
  export CMAKE_OSX_SYSROOT=/tmp/MacOSX-SDKs/MacOSX10.9.sdk
}

# ///////////////////////////////////////////////
function CreateNonGuiVersion() {
   mkdir -p Release.nogui
   cp -R   Release/OpenVisus Release.nogui/OpenVisus
   rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# //////////////////////////////////////////////////////////////
function ConfigureAndTestConda() {
   conda develop $PWD/..
   $PYTHON -m OpenVisus configure || true # this can fail on linux
   $PYTHON -m OpenVisus test
   $PYTHON -m OpenVisus test-gui    || true # this can fail on linux
   conda develop $PWD/.. uninstall
}

# //////////////////////////////////////////////////////////////
function DistribToConda() {
   rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2") || true
   $PYTHON setup.py -q bdist_conda 1>/dev/null
   CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2" | head -n 1)
   if [[ "${GIT_TAG}" != "" ]] ; then
      anaconda --verbose --show-traceback -t ${ANACONDA_TOKEN} upload ${CONDA_FILENAME}
   fi
}

# install prerequisistes
InstallSDK

# install conda
if [[ "1" == "1" ]]; then
  export PYTHONNOUSERSITE=True  # avoid conflicts with pip packages installed using --user
  InstallConda
  ActivateConda
  PYTHON=`which python`
fi

# compile openvisus
if [[ "1" == "1" ]]; then
  BUILD_DIR=build_macos_conda
  mkdir -p ${BUILD_DIR} 
  cd ${BUILD_DIR}
  cmake  \
    -GXcode \
    -DQt5_DIR=${CONDA_PREFIX}/lib/cmake/Qt5 \
    -DCMAKE_OSX_SYSROOT=$CMAKE_OSX_SYSROOT \
    -DPython_EXECUTABLE=${PYTHON} \
    -DVISUS_GUI=$VISUS_GUI \
    -DVISUS_SLAM=$VISUS_SLAM \
    -DVISUS_MODVISUS=$VISUS_MODVISUS \
    ../
  cmake --build . --target ALL_BUILD --config Release --parallel 4
  cmake --build . --target install	 --config Release 
fi

# for for `bdist_conda` problem
if [[ "1" == "1" ]]; then
	pushd ${CONDA_PREFIX}/lib/python${PYTHON_VERSION}
	cp -n distutils/command/bdist_conda.py site-packages/setuptools/_distutils/command/bdist_conda.py || true
	popd
fi

if [[ "1" == "1" ]]; then
  pushd Release/OpenVisus
  ConfigureAndTestConda  
  DistribToConda  
  popd
fi

if [[ "$VISUS_GUI" == "1" ]]; then
  CreateNonGuiVersion
  pushd Release.nogui/OpenVisus
  ConfigureAndTestConda
  DistribToConda
  popd
fi

echo "All done macos conda $PYTHON_VERSION} "