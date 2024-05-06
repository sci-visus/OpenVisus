
# How to build OpenVisus

Table of content:

- [Windows compilation using Visual Studio](#windows-compilation-visual-studio)
- [Windows compilation using mingw](#windows-compilation-mingw)
- [MacOSX compilation using clang](#macosx-compilation-clang)
- [MacOSX compilation using gcc](#macosx-compilation-gcc)
- [MaxODX compulation arm64](#macosx-compilation-arm64)
- [Linux compilation using gcc](#linux-compilation-gcc)
- [Minimal compilation](#minimal-compilation)
- [Commit tag](#commit-tag)

## Windows compilation Visual Studio

Install git, cmake and swig.  
The fastest way is to use `chocolatey`:

```
choco install -y git cmake swig
```

Install Python3.x.

Install Qt5 (http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe)

To compile OpenVisus (change the paths as needed):

```
set Python_EXECUTABLE=C:\Python37\python.exe
set Qt5_DIR=D:\Qt\5.12.8\5.12.8\msvc2017_64\lib\cmake\Qt5

python -m pip install numpy

git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build
cd build
cmake -G "Visual Studio 17 2022" -A "x64" -DQt5_DIR=%Qt5_DIR% -DPython_EXECUTABLE=%Python_EXECUTABLE% ../ 
cmake --build . --target ALL_BUILD --config Release
cmake --build . --target INSTALL   --config Release

set PYTHON_PATH=.\Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```



<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation clang

Make sure you have command line tools:

```
sudo xcode-select --install || sudo xcode-select --reset
```

Build the repository (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# change as needed if you have python in another place
Python_EXECUTABLE=/Library/Frameworks/Python.framework/Versions/3.6/bin/python3

# install prerequisites
brew install swig cmake

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build 
cd build

cmake -GXcode -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4
cmake --build ./ --target install   --config Release

export PYTHONPATH=$(pwd)/Release

# this command will install PyQt5 and link OpenVisus to PyQt5 in user space (given that you don't want to run as root)
python3 -m OpenVisus configure --user
python3 -m OpenVisus test
python3 -m OpenVisus viewer

# OPTIONAL
python3 -m pip install --upgrade opencv-python opencv-contrib-python 
python3 -m OpenVisus test-viewer1
python3 -m OpenVisus test-viewer2

# OPTIONAL
python3 -m pip install --upgrade jupyter
python3 -m jupyter notebook ../Samples/jupyter/Agricolture.ipynb
```


<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation gcc

Maybe you need to install gcc:

```
brew install gcc@9
```

Build the repository (change as needed):

```

# change the path for your gcc
export CC=$(brew --prefix gcc@9)/bin/gcc-9
export CXX=$(brew --prefix gcc@9)/bin/g++-9


git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# change as needed if you have python in another place
Python_EXECUTABLE=/Library/Frameworks/Python.framework/Versions/3.6/bin/python3

# install prerequisites
brew install swig cmake

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build_gcc
cd build_gcc
cmake -G"Unix Makefiles" -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all       --config Release --parallel 8
cmake --build ./ --target install   --config Release

export PYTHONPATH=$(pwd)/Release

# this command will install PyQt5 and link OpenVisus to PyQt5 in user space (given that you don't want to run as root)
python3 -m OpenVisus configure --user
python3 -m OpenVisus test
python3 -m OpenVisus viewer

# OPTIONAL
python3 -m pip install --upgrade opencv-python opencv-contrib-python 
python3 -m OpenVisus viewer1
python3 -m OpenVisus viewer2

# OPTIONAL
python3 -m pip install --upgrade jupyter
python3 -m jupyter notebook ../Samples/jupyter/Agricolture.ipynb
```

## MacOSX compilation arm64


# Instructions

Create a Silocon M1/M2 machine(e.g.  Scaleway or MacCloud)

- change shell to `bash``
- setup `id_nsdf`` key (it will be the default `id_rsa`)
- add the pub key to `~/.ssh/authorized_keys`
- install xcode
- Open xcode once from the UI

Create the `~/.bashrc` file with credentials to upload artifacts:

```bash
export ANACONDA_TOKEN="xxx"
export PYPI_USERNAME="__token__"
export PYPI_TOKEN="zzzz" # generate a new token with the name `arm64`
```

Remove request for sudo password (needed to run cpython script):

```
sudo visudo

<your-user-name-here> ALL=(ALL) NOPASSWD: ALL
```

Clone the OpenVisus repo

```bash
mkdir -p sci-visus
cd sci-visus
git clone git clone git@github.com:sci-visus/OpenVisus.git
cd OpenVisus

git pull

./scripts/arm64.all.sh


```

<!--//////////////////////////////////////////////////////////////////////// -->
## Linux compilation gcc

We are showing as an example how to build OpenVisus on Ubuntu 16.

Install prerequisites:

```
sudo apt install -y patchelf swig cmake
```

Install python3 (>=3.6):

```
sudo apt install -y python3 python3-dev python3-pip
python3 -m pip install numpy
```

(OPTIONAL) Install apache developer files if you want to enable Apache mod_visus:

```
sudo apt-get install -y libapr1 libapr1-dev libaprutil1 libaprutil1-dev apache2-dev
```

(OPTIONAL) Install qt5 if you want to compile the visus viewer (change repository and version as needed):

```
sudo add-apt-repository -y ppa:beineri/opt-qt-5.12.8-xenial
sudo apt update
sudo apt-get install -y qt512base qt512imageformats
```

Compile OpenVisus (change options -D as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build && cd build

cmake -DPython_EXECUTABLE=$(which python3) -DQt5_DIR=/opt/qt512/lib/cmake/Qt5 -DVISUS_GUI=0 _DVISUS_MODVISUS=0 ../
cmake --build ./ --target all     --config Release --parallel 4 
cmake --build ./ --target install --config Release
```

Test from command line:

```
# test embedding python
Release/OpenVisus/bin/visus

# test extending python
PYTHONPATH=$(pwd)/Release python3 -c "from OpenVisus import *"
```
If you want to test the viewer:

```
PYTHONPATH=$(pwd)/Release python3 -m OpenVisus configure --user # configure is needed to install PyQt5
PYTHONPATH=$(pwd)/Release python3 -m OpenVisus viewer
```


<!--//////////////////////////////////////////////////////////////////////// -->
## Commit tag

For OpenVisus developers only:

```
TAG=$(python3 Libs/swig/setup.py new-tag) && echo ${TAG}
git commit -a -m "New tag" && \
  git tag -a $TAG -m "$TAG" && \
  git push origin $TAG && \
  git push origin
```

Also remember to manually change the version in [environment.yml](https://github.com/sci-visus/OpenVisus/blob/master/environment.yml) that is used in binder.

