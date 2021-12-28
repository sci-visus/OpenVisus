#!/bin/bash

set -e
set -x

source scripts/build_utils.sh

# install swig
curl -L --insecure https://cfhcable.dl.sourceforge.net/project/swig/swigwin/swigwin-4.0.2/swigwin-4.0.2.zip -O 
unzip swigwin-4.0.2.zip

# (DISABLED) install ospray
# git clone https://github.com/sci-visus/ospray_win.git ExternalLibs/ospray_win

# configure conda
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no             1>/dev/null
conda install -c conda-forge -y conda cmake pyqt=5.12  anaconda-client  conda-build wheel 1>/dev/null

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`
PYTHON=`which python`
BUILD_DIR=build        

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR}
cmake -G "Visual Studio 16 2019" -A x64 -DQt5_DIR=${CONDA_PREFIX}/Library/lib/cmake/Qt5 -DPython_EXECUTABLE=${PYTHON} -DSWIG_EXECUTABLE=../swigwin-4.0.2/swig.exe \
    -DVISUS_GUI=0 -DVISUS_SLAM=0 -DVISUS_MODVISSU=0 \
    ../ 
cmake --build . --target ALL_BUILD --config Release --parallel 4
cmake --build . --target install   --config Release

CreateNonGuiVersion

# ////////////////////////////////////////////////////////////// !!!REMOVE !!!!
function DistribToConda() {

    if [[ ! -f QT_VERSION ]] ; then 
        PACKAGE_NAME=openvisusnogui
    else
        PACKAGE_NAME=openvisus
    fi

    # The -f option forces a reinstall if a previous version of the package was already installed.
    cat <<EOF > meta.yaml
package:
    name: $PACKAGE_NAME
    version: "$GIT_TAG"
source:
    path: .
build:
    script: python setup.py install -f 
requirements:
    build:
        - python
        - setuptools
    run:
        - python
EOF

    rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2")  || true \
    # 
    # $PYTHON -v setup.py  bdist_conda (BROKEN, always problems with bdist_conda)
    conda build .
    CONDA_FILENAME=`find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"    | head -n 1` 
    if [[ "${GIT_TAG}" != ""    ]] ; then
        anaconda --verbose --show-traceback -t ${ANACONDA_TOKEN} upload ${CONDA_FILENAME}
    fi
}       

pushd Release/OpenVisus
ConfigureAndTestConda
cp -n $CONDA_PREFIX/Lib/distutils/command/bdist_conda.py $CONDA_PREFIX/Lib/site-packages/setuptools/bdist_conda.py
ls $CONDA_PREFIX/Lib/site-packages/setuptools/bdist_conda.py
DistribToConda
popd

pushd Release.nogui/OpenVisus
ConfigureAndTestConda
cp -n $CONDA_PREFIX/Lib/distutils/command/bdist_conda.py $CONDA_PREFIX/Lib/site-packages/setuptools/bdist_conda.py
ls $CONDA_PREFIX/Lib/site-packages/setuptools/bdist_conda.py
DistribToConda
popd

