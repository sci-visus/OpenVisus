# Instructions

Create a Silocon M1/M2 machine on Scaleway

- change shell to bash
- setup `id_nsdf`` key (it will be the default `id_rsa``)
- add the pub key to `~/.ssh/authorized_keys`

Install XCode
- Open and agrees with conditions otherwise it will not work from command line

Create the `~/.bashrc` file with something like:

```bash
ulimit -n 4096
export PS1="\[\033[49;1;31m\][\t] [\u@\h] [\w]  \[\033[49;1;0m\] \n"
alias l='ls --color -la'
alias more='less'
export ANACONDA_TOKEN="xxx"
export PYPI_USERNAME="__token__"
export PYPI_TOKEN="zzzz" # generate a new token with the name `arm64`
```

Setup the machine

```bash

# install homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# install miniforge3 for arm64
pushd ~
curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-arm64.sh
bash Miniforge3-MacOSX-arm64.sh -b # silent
rm -f Miniforge3-MacOSX-x86_64.sh
popd
~/miniforge3/bin/conda config --set always_yes yes --set anaconda_upload no

# install a portable SDK (it sems the same used by miniforge and  PyQt5-5.15.10-cp37-abi3-macosx_11_0_arm64.whl)
pushd /tmp 
rm -Rf MacOSX-SDKs 
git clone https://github.com/phracker/MacOSX-SDKs.git
popd
sudo ln -s /tmp/MacOSX-SDKs/MacOSX11.3.sdk /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/

# clone the OpenVisus repo
cd ~
mkdir -p sci-visus
cd sci-visus
git clone git clone git@github.com:sci-visus/OpenVisus.git
cd OpenVisus
```

## Setup CPython (once only)

Do once possibily in a separated terminal

```bash

# enable brew
eval "$(/opt/homebrew/bin/brew shellenv)"

# check the environment
# brew config  # ..macOS: 13.5-arm64 ...

brew install swig cmake qt@5
```

Fix qt5 brew problem

Do a `code /opt/homebrew/opt/qt@5/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake`  and replace the first line with this:

```bash
set(_GL_INCDIRS "/System/Library/Frameworks/OpenGL.framework/Headers" "/System/Library/Frameworks/AGL.framework/Headers" "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks")
```

# CPYTHON

```bash

function compile_cpython() {

  PYTHON_VERSION=${1}

  BUILD_DIR="./build-cpython-${PYTHON_VERSION}"
  PYTHON_EXE=`which python${PYTHON_VERSION}`
  PYTHON_TAG=cp${PYTHON_VERSION/./}

  ${PYTHON_EXE} -m pip install --user setuptools wheel twine --upgrade 

  mkdir -p ${BUILD_DIR}
  pushd ${BUILD_DIR}

  # make sure you have a good pip
  ${PYTHON_EXE} -m pip install --upgrade pip

  # compile openvisus
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

  # GUI VERSION
  pushd ./Release/OpenVisus
  export PYTHONPATH=../
  ${PYTHON_EXE} -m OpenVisus configure --user
  ${PYTHON_EXE} -m OpenVisus test
  ${PYTHON_EXE} -m OpenVisus test-gui 
  unset PYTHONPATH
  rm -Rf ./dist
  
  ${PYTHON_EXE} setup.py -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=${PYPI_PLATFORM_NAME}
  ${PYTHON_EXE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing "dist/*.whl" 
  popd

  # NO GUI VERSION
  mkdir -p Release.nogui
  cp -R Release/OpenVisus Release.nogui/OpenVisus
  rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
  pushd Release.nogui/OpenVisus
  export PYTHONPATH=../
  ${PYTHON_EXE} -m OpenVisus configure
  ${PYTHON_EXE} -m OpenVisus test
  ${PYTHON_EXE} -m OpenVisus test-gui
  unset PYTHONPATH
  rm -Rf ./dist

  ${PYTHON_EXE} setup.py -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=${PYPI_PLATFORM_NAME}
  ${PYTHON_EXE} -m twine upload --username "${PYPI_USERNAME}" --password "${PYPI_TOKEN}" --skip-existing "dist/*.whl" 
  popd 

  popd
}

bash

# enable brew
eval "$(/opt/homebrew/bin/brew shellenv)"

brew install python@3.8
brew install python@3.9
brew install python@3.10
brew install python@3.11
brew install python@3.12

# all needed for apple arm compilation
export CODE_SIGN=/usr/bin/codesign
export PYPI_PLATFORM_NAME="macosx_11_0_arm64" 
export CMAKE_OSX_ARCHITECTURES="arm64"
export CMAKE_OSX_SYSROOT="/tmp/MacOSX-SDKs/MacOSX11.3.sdk"
export QT5_DIR="/opt/homebrew/opt/qt@5/lib/cmake/Qt5"

cd ~/sci-visus/OpenVisus
rm -Rf ./build-cpython*

compile_cpython 3.8
compile_cpython 3.9
compile_cpython 3.10
compile_cpython 3.11
compile_cpython 3.12
```

# Conda

```bash

function compile_conda() {

  PYTHON_VERSION=${1}

  mamba create --name my-env-${PYTHON_VERSION} -c conda-forge python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel pyqt swig cmake
  conda activate my-env-${PYTHON_VERSION}

  # avoid conflicts with pip packages installed using --user
  conda env config vars set PYTHONNOUSERSITE=True
  
  # check the environment
  # which python                                                  # /Users/m1/miniforge3/bin/python
  # python -c "import platform; print(platform.processor())"      # arm
  # python -c "import sysconfig;print(sysconfig.get_platform())"  # macosx-11.0-arm64

  mkdir -p build-conda-${PYTHON_VERSION}
  cd build-conda-${PYTHON_VERSION}
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

  pushd Release/OpenVisus

  # GUI version
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

  # NO GUI version
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
}

source ~/miniforge3/bin/activate

# tools to change the RPATH (NOTE: conda tools causing crashes, so using OS tools)
export INSTALL_NAME_TOOL=/usr/bin/install_name_tool  
export CODE_SIGN=/usr/bin/codesign
export PYPI_PLATFORM_NAME="macosx_11_0_arm64" 
export CMAKE_OSX_ARCHITECTURES="arm64"
export CMAKE_OSX_SYSROOT="/tmp/MacOSX-SDKs/MacOSX11.3.sdk"

rm -Rf ~/miniforge3/envs/my-env*

cd ~/sci-visus/OpenVisus
rm -Rf ./build-conda*

compile_conda 3.8
compile_conda 3.9
compile_conda 3.10
compile_conda 3.11

# NOT available yet?
# compile_conda 3.12

```
