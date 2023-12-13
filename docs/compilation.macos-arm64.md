# Instructions

Create a Siliocon M1/M2 machine on Scaleway
- change shell to bash
- setup `id_nsdf`` key (it will be the default `id_rsa``)
- add the pub key to `~/.ssh/authorized_keys`

Install XCode
- Open and agrees otherwise it will not work

Open a bash shell:

```bash

# avoid conflicts with pip packages installed using --user
export PYTHONNOUSERSITE=True  

# install miniforge3 for arm64
pushd ~
curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-MacOSX-arm64.sh
bash Miniforge3-MacOSX-arm64.sh -b # silent
rm -f Miniforge3-MacOSX-x86_64.sh
popd

source ~/miniforge3/bin/activate
conda config --set always_yes yes --set anaconda_upload no

# install the SDK (it's the same used by miniforge)
# also py PyQt PyQt5-5.15.10-cp37-abi3-macosx_11_0_arm64.whl
pushd /tmp
curl -sL -O  "https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX11.0.sdk.tar.xz" 
tar xvzf MacOSX11.0.sdk.tar.xz
popd

mkdir -p sci-visus
cd sci-visus
git clone git clone git@github.com:sci-visus/OpenVisus.git
cd OpenVisus

mamba create --name my-env -c conda-forge python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel pyqt swig cmake
conda activate my-env

which python
# /Users/m1/miniforge3/bin/python

python3 -c "import platform; print(platform.processor())"
# arm

python3 -c "import sysconfig;print(sysconfig.get_platform())"
# will tell you `macosx-11.0-arm64`

# compile openvisus
mkdir -p build
cd build


cmake  \
  -GXcode \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_SYSROOT=/tmp/MacOSX11.0.sdk \
  -DQt5_DIR=${CONDA_PREFIX}/lib/cmake/Qt5 \
  -DPython_EXECUTABLE=`which python` \
  -DVISUS_GUI=1 \
  -DVISUS_SLAM=0 \
  -DVISUS_MODVISUS=0 \
  -DVISUS_IMAGE=0 \
  ../

cmake --build . --target ALL_BUILD --config Release
cmake --build . --target install   --config Release 

# ok
pushd Release/OpenVisus

# then change rpath (NOTE: some tools coming from conda, produce invalid binaries, this is why we are using /usr/bin)
export INSTALL_NAME_TOOL=/usr/bin/install_name_tool 
export CODE_SIGN=/usr/bin/codesign

# do the real configuration (add --dry-run to see what's going on)
export PYTHONPATH=$PWD/..
python -m OpenVisus configure
python -m OpenVisus test
python -m OpenVisus test-gui
unset PYTHONPATH

rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2") || true
python setup.py -q bdist_conda
CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2" | head -n 1)
anaconda --verbose --show-traceback -t "${ANACONDA_TOKEN}" upload "${CONDA_FILENAME}"

popd

# no-gui version
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

```



# CPYTHON

```

# install homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# enable brew
eval "$(/opt/homebrew/bin/brew shellenv)"
brew config 
# ..macOS: 13.5-arm64 ...

pushd /tmp 
rm -Rf MacOSX-SDKs 
git clone https://github.com/phracker/MacOSX-SDKs.git 1>/dev/null
popd

brew install swig cmake 

# brew install python@${PYTHON_VERSION}


python3 -c "import platform; print(platform.processor())"
# arm

python3 -c "import sysconfig;print(sysconfig.get_platform())"
# macosx-10.9-universal2

python3 -m pip install --upgrade pip

brew install qt@5

# code /opt/homebrew/opt/qt@5/lib/cmake/Qt5Gui/Qt5GuiConfigExtras.cmake
# replace the first line with this
# set(_GL_INCDIRS "/System/Library/Frameworks/OpenGL.framework/Headers" "/System/Library/Frameworks/AGL.framework/Headers" "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks")

# /opt/homebrew/opt/qt@5/lib
# Qt5_Dir=/tmp/Qt/5.15.0/clang_64/lib/cmake/Qt5

# compile openvisus
mkdir -p build-cpython
cd build-cpython

#  -DCMAKE_OSX_SYSROOT=/tmp/MacOSX-SDKs/MacOSX11.0.sdk \
cmake \
  -GXcode \
  -DQt5_DIR=/opt/homebrew/opt/qt@5/lib/cmake/Qt5 \
  -DPython_EXECUTABLE=`which python3` \
  -DVISUS_GUI=1 \
  -DVISUS_SLAM=0 \
  -DVISUS_MODVISUS=0 \
  ../
cmake --build . --target ALL_BUILD --config Release 
cmake --build . --target install   --config Release

export CODE_SIGN=/usr/bin/codesign


pushd Release/OpenVisus
export PYTHONPATH=../
python3   -m OpenVisus configure 
python3   -m OpenVisus test
python3   -m OpenVisus test-gui 
unset PYTHONPATH

rm -Rf ./dist
python3 -m pip install setuptools wheel twine --upgrade 
python3 setup.py -q bdist_wheel --python-tag=cp39 --plat-name=macosx_11_0_arm64
python3 -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing   "dist/*.whl" 

popd


mkdir -p Release.nogui
cp -R   Release/OpenVisus Release.nogui/OpenVisus
rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
pushd Release.nogui/OpenVisus

export PYTHONPATH=../
python3   -m OpenVisus configure || true # this can fail on linux
python3   -m OpenVisus test
python3   -m OpenVisus test-gui || true # this can fail on linux
unset PYTHONPATH

rm -Rf ./dist
python3 -m pip install setuptools wheel twine --upgrade 
python3 setup.py -q bdist_wheel --python-tag=cp39 --plat-name=macosx_11_0_arm64
python3 -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing   "dist/*.whl" 

popd 


```