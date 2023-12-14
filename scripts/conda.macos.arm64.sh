
#!/bin/bash

set -ex

PYTHON_VERSION=${1:-3.9}

# install a portable SDK (it sems the same used by miniforge and  PyQt5-5.15.10-cp37-abi3-macosx_11_0_arm64.whl)
if [ ! -d "/tmp/MacOSX-SDKs" ] ; then
  pushd /tmp 
  rm -Rf MacOSX-SDKs 
  git clone https://github.com/phracker/MacOSX-SDKs.git
  popd
  sudo ln -s /tmp/MacOSX-SDKs/MacOSX11.3.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
fi

# install miniforge3 for arm64
if [ ! -d "${HOME}/miniforge3" ] ; then
  pushd ~
  curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-arm64.sh
  bash Miniforge3-MacOSX-arm64.sh -b # silent
  rm -f Miniforge3-MacOSX-x86_64.sh
  popd
  ~/miniforge3/bin/conda config --set always_yes yes --set anaconda_upload no
fi

# create and activate conda environment
if [[ 1 == 1 ]] ; then
  source ~/miniforge3/bin/activate
  ENV_NAME=arm64-env-${PYTHON_VERSION}

  # conda env remove --name ${ENV_NAME}
  if [ ! -d "${HOME}/miniforge3/envs/${ENV_NAME}" ] ; then
    mamba create --name ${ENV_NAME} -c conda-forge python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel pyqt swig cmake
  fi

  source ${HOME}/miniforge3/envs/${ENV_NAME}/bin/activate 
  # conda activate ${ENV_NAME}

  # patch qt5 for arm64
  if [[ 1 == 1 ]] ; then
    export _expr_a='set(_GL_INCDIRS "/System/Library/Frameworks/OpenGL.framework/Headers" "/System/Library/Frameworks/AGL.framework/Headers")'
    export _expr_b='set(_GL_INCDIRS "/System/Library/Frameworks/OpenGL.framework/Headers" "/System/Library/Frameworks/AGL.framework/Headers" "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks")'
    __to_patch_filename="${CONDA_PREFIX}/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake"
    cat ${__to_patch_filename} | python -c "import os,sys;print(sys.stdin.read().replace(os.environ['_expr_a'],os.environ['_expr_b']))" > ${__to_patch_filename}
  fi
 
fi

# avoid conflicts with pip packages installed using --user
# tools to change the RPATH (NOTE: conda tools causing crashes, so using OS tools)
conda env config vars set PYTHONNOUSERSITE=True
conda env config vars set INSTALL_NAME_TOOL=/usr/bin/install_name_tool  
conda env config vars set CODE_SIGN=/usr/bin/codesign
conda env config vars set PYPI_PLATFORM_NAME="macosx_11_0_arm64" 
conda env config vars set CMAKE_OSX_ARCHITECTURES="arm64"
conda env config vars set CMAKE_OSX_SYSROOT="/tmp/MacOSX-SDKs/MacOSX11.3.sdk"

# check the environment
# which python                                                  # /Users/m1/miniforge3/bin/python
# python -c "import platform; print(platform.processor())"      # arm
# python -c "import sysconfig;print(sysconfig.get_platform())"  # macosx-11.0-arm64


BUILD_DIR="./build-conda-${PYTHON_VERSION}"
# rm -Rf ${BUILD_DIR}
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

if [[ 1 == 1 ]] ; then

  cmake  \
    -GXcode \
    -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} \
    -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} \
    -DQt5_DIR=${CONDA_PREFIX}/lib/cmake/Qt5 \
    -DPython_EXECUTABLE=`which python` \
    -DVISUS_GUI=1 \
    -DVISUS_SLAM=0 \
    -DVISUS_MODVISUS=0 \
    -DVISUS_IMAGE=0 \
    ../

  cmake --build . --target ALL_BUILD --config Release
  cmake --build . --target install   --config Release
fi

# GUI version
if [[ 1 == 1 ]] ; then

  pushd Release/OpenVisus

  export PYTHONPATH=$PWD/..
  python -m OpenVisus configure #  add `--dry-run`` to see what's going on
  python -m OpenVisus test
  python -m OpenVisus test-gui
  unset PYTHONPATH

  rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2") || true
  python setup.py -q bdist_conda
  CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2" | head -n 1)
  anaconda --verbose --show-traceback -t "${ANACONDA_TOKEN}" upload "${CONDA_FILENAME}"
  
  popd
fi

# NO GUI version
if [[ 1 == 1 ]] ; then
  mkdir -p Release.nogui
  cp -R   Release/OpenVisus Release.nogui/OpenVisus
  rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")

  pushd Release.nogui/OpenVisus

  export PYTHONPATH=$PWD/..
  python -m OpenVisus configure
  python -m OpenVisus test
  unset PYTHONPATH

  rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2") || true
  python setup.py -q bdist_conda 
  CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2" | head -n 1)
  anaconda --verbose --show-traceback -t ${ANACONDA_TOKEN} upload ${CONDA_FILENAME}

  popd
fi