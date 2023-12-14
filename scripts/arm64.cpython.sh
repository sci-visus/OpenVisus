#!/bin/bash

set -ex

PYTHON_VERSION=${PYTHON_VERSION:-3.9}

# all needed for apple arm compilation
export PYTHON_TAG=cp${PYTHON_VERSION/./}
export CODE_SIGN=/usr/bin/codesign
export PYPI_PLATFORM_NAME="macosx_11_0_arm64" 
export CMAKE_OSX_ARCHITECTURES="arm64"


# install a portable SDK (it sems the same used by miniforge and  PyQt5-5.15.10-cp37-abi3-macosx_11_0_arm64.whl)
if [ ! -d "/tmp/MacOSX-SDKs" ] ; then
  pushd /tmp 
  rm -Rf MacOSX-SDKs 
  git clone https://github.com/phracker/MacOSX-SDKs.git
  popd
  sudo ln -s /tmp/MacOSX-SDKs/MacOSX11.3.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/
fi
export CMAKE_OSX_SYSROOT="/tmp/MacOSX-SDKs/MacOSX11.3.sdk"

# install homebrew
if [ ! -d /opt/homebrew ] ; then
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
fi
eval "$(/opt/homebrew/bin/brew shellenv)"

# install dependencies
if [[ 1 == 1 ]] ; then
   brew install python@${PYTHON_VERSION} swig cmake || true

   # use a virtual environment
   if [[ 1 == 1 ]] ; then
      ENV_NAME=arm64-env-${PYTHON_VERSION}
      python${PYTHON_VERSION} -m pip install virtualenv || true
      python${PYTHON_VERSION} -m venv ${HOME}/cpython/envs/${ENV_NAME}
      source  ${HOME}/cpython/envs/${ENV_NAME}/bin/activate
   fi

   PYTHON_EXE=`which python${PYTHON_VERSION}`
   echo "PYTHON_EXE=${PYTHON_EXE}"

   # make sure you have a good pip
   ${PYTHON_EXE} -m pip install --upgrade pip

   brew install qt@5

   # patch qt5 for arm64
   if [[ 1 == 1 ]] ; then
      export _expr_a='set(_GL_INCDIRS "/System/Library/Frameworks/OpenGL.framework/Headers" "/System/Library/Frameworks/AGL.framework/Headers")'
      export _expr_b='set(_GL_INCDIRS "/System/Library/Frameworks/OpenGL.framework/Headers" "/System/Library/Frameworks/AGL.framework/Headers" "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks")'
      __to_patch_filename="/opt/homebrew/opt/qt@5/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake"
      sudo chmod a+w ${__to_patch_filename} # need password-less sudo here
      cat ${__to_patch_filename} | ${PYTHON_EXE} -c "import os,sys;print(sys.stdin.read().replace(os.environ['_expr_a'],os.environ['_expr_b']))" > ${__to_patch_filename}
   fi

   export QT5_DIR="/opt/homebrew/opt/qt@5/lib/cmake/Qt5"

   ${PYTHON_EXE} -m pip install setuptools wheel twine --upgrade 
fi

#  cjeck python environment
echo "PYTHON_EXE=${PYTHON_EXE}"
${PYTHON_EXE} -c "import platform; print(platform.processor())"      # arm
${PYTHON_EXE} -c "import sysconfig;print(sysconfig.get_platform())"  # macosx-11.0-arm64


BUILD_DIR="./build-cpython-${PYTHON_VERSION}"
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# compile openvisus
if [[ 1 == 1 ]] ; then

   cmake \
      -GXcode \
      -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} \
      -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} \
      -DQt5_DIR=${QT5_DIR} \
      -DPython_EXECUTABLE=${PYTHON_EXE} \
      -DVISUS_GUI=1 \
      -DVISUS_SLAM=0 \
      -DVISUS_MODVISUS=0 \
      ../
   
   cmake --build . --target ALL_BUILD --config Release 
   cmake --build . --target install   --config Release

fi

# GUI VERSION
if [[ 1 == 1 ]] ; then

   pushd ./Release/OpenVisus

   export PYTHONPATH=../
   ${PYTHON_EXE} -m OpenVisus configure 
   ${PYTHON_EXE} -m OpenVisus test
   ${PYTHON_EXE} -m OpenVisus test-gui 
   unset PYTHONPATH

   rm -Rf ./dist
   ${PYTHON_EXE} setup.py -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=${PYPI_PLATFORM_NAME}
   ${PYTHON_EXE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing "dist/*.whl" 

   popd

fi

# NO GUI VERSION
if [[ 1 == 1 ]] ; then

   mkdir -p Release.nogui
   cp -R Release/OpenVisus Release.nogui/OpenVisus
   rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")

   pushd Release.nogui/OpenVisus

   export PYTHONPATH=../
   ${PYTHON_EXE} -m OpenVisus configure
   ${PYTHON_EXE} -m OpenVisus test
   unset PYTHONPATH

   rm -Rf ./dist
   ${PYTHON_EXE} setup.py -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=${PYPI_PLATFORM_NAME}
   ${PYTHON_EXE} -m twine upload --username "${PYPI_USERNAME}" --password "${PYPI_TOKEN}" --skip-existing "dist/*.whl" 

   popd 
fi



