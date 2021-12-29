#!/bin/bash

set -ex

PYTHON_VERSION=${PYTHON_VERSION:-3.8}
PYQT_VERSION=${PYQT_VERSION:-5.12}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-0}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# ///////////////////////////////////////////////
function InstallConda() {
    choco install --accept-license --yes --force miniconda3 
    CONDA_HOME=/c/tools/miniconda3
    echo "source ${CONDA_HOME}/etc/profile.d/conda.sh" >> ~/.bashrc
}

# ///////////////////////////////////////////////
function ActivateConda() {
    source ~/.bashrc
    conda config  --set always_yes yes --set changeps1 no --set anaconda_upload no 
    conda create --name my-env -c conda-forge python=${PYTHON_VERSION} numpy cmake swig pyqt=${PYQT_VERSION} anaconda-client wheel conda conda-build
    conda activate my-env
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

# install prerequisites
# (DISABLED) install ospray
# git clone https://github.com/sci-visus/ospray_win.git ExternalLibs/ospray_win

# install conda
if [[ "1" == "1" ]]; then
    export PYTHONNOUSERSITE=True  # avoid conflicts with pip packages installed using --user
    InstallConda
    ActivateConda
    PYTHON=`which python`
fi


# compile openvisus
if [[ "1" == "1" ]]; then
    BUILD_DIR=build_windows_conda
    mkdir -p ${BUILD_DIR} 
    cd ${BUILD_DIR}
    cmake -G "Visual Studio 16 2019" -A x64 \
        -DQt5_DIR=${CONDA_PREFIX}/Library/lib/cmake/Qt5 \
        -DSWIG_EXECUTABLE=$(which swig) \
        -DPython_EXECUTABLE=${PYTHON} \
        -DVISUS_GUI=${VISUS_GUI} \
        -DVISUS_SLAM=${VISUS_SLAM} \
        -DVISUS_MODVISUS=${VISUS_MODVISUS} \
        ../ 
    cmake --build . --target ALL_BUILD --config Release --parallel 8
    cmake --build . --target install   --config Release
fi

# for for `bdist_conda` problem
if [[ "1" == "1" ]]; then
    find ${CONDA_PREFIX} # REMOVE ME!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    pushd ${CONDA_PREFIX}/Lib
    cp -n distutils/command/bdist_conda.py site-packages/setuptools/_distutils/command/bdist_conda.py || true # just le'ts hope it will work anyway
    popd
fi

if [[ "1" == "1" ]]; then
    pushd Release/OpenVisus
    ConfigureAndTestConda
    DistribToConda
    popd
fi

if [[ "${VISUS_GUI}" == "1" ]]; then
    CreateNonGuiVersion
    pushd Release.nogui/OpenVisus 
    ConfigureAndTestConda 
    DistribToConda 
    popd
fi


echo "All done"
