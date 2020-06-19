# OpenViSUS Visualization project  

![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)
[![Binder](https://mybinder.org/badge_logo.svg)](https://mybinder.org/v2/gh/sci-visus/OpenVisus/master?filepath=Samples%2Fjupyter)


The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license.

Table of content:

- [Binary Distribution](#binary-distribution)

- [Windows compilation](#windows-compilation)

- [MacOSX compilation](#macosx-compilation)

- [Linux compilation](#linux-compilation)

- [Minimal compilation](#minimal-compilation)


<!--//////////////////////////////////////////////////////////////////////// -->
## Binary distribution

If you are using `pip`

```
# For Linux sometimes you have to install some python libraries 
# sudo apt-get install python3.6 libpython3/6

python -m pip install --user --upgrade OpenVisus
python -m OpenVisus configure --user
python -m OpenVisus viewer
```

If you are using `conda`:

```
conda install --yes --channel visus openvisus
python -m OpenVisus configure 
python -m OpenVisus viewer
```

Give a look to directory `Samples/python` and Jupyter examples:

[Samples/jupyter/quick_tour.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/quick_tour.ipynb)

[Samples/jupyter/Agricolture.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Agricolture.ipynb)

[Samples/jupyter/Climate.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/Climate.ipynb)

[Samples/jupyter/ReadAndView.ipynb](https://github.com/sci-visus/OpenVisus/blob/master/Samples/jupyter/ReadAndView.ipynb)


<!--//////////////////////////////////////////////////////////////////////// -->
## Windows compilation

Install git, cmake and swig.  
The fastest way is to use `chocolatey`:

```
choco install -y git cmake swig
```

Install Python3.x.

Install Qt5 (http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe)

To compile OpenVisus (change as needed):

```
set Python_EXECUTABLE=<FillHere>
set Qt5_DIR=</FillHere>

python -m pip install numpy

git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR=%Qt5_DIR% -DPython_EXECUTABLE=%Python_EXECUTABLE% ../ 
cmake --build . --target ALL_BUILD --config Release
cmake --build . --target INSTALL   --config Release

set PYTHON_PATH=.\Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```

<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation

Make sure you have command line tools:

```
sudo xcode-select --install || sudo xcode-select --reset
```


Build the repository (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# update submodules
git pull --recurse-submodules

# change as needed if you have python in another place
Python_ROOT_DIR=/Library/Frameworks/Python.framework/Versions/3.6

# install qt5 (change as needed)
brew install qt5
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5

mkdir build 
cd build

brew install swig cmake
cmake -GXcode -DPython_ROOT_DIR=${Python_ROOT_DIR} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4
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

<!--//////////////////////////////////////////////////////////////////////// -->
## Linux compilation

We are showing as an example how to build OpenVisus on Ubuntu 16.

Install prerequisites:

```
sudo apt install -y patchelf swig
```

Install a recent cmake. For example:

```
wget https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-Linux-x86_64.sh
sudo mkdir /opt/cmake
sudo sh cmake-3.17.2-Linux-x86_64.sh --skip-license --prefix=/opt/cmake
sudo ln -s /opt/cmake/bin/cmake /usr/bin/cmake
```

Install python (choose the version you prefer, here we are assuming 3.7):

```
sudo apt update
sudo apt install -y software-properties-common
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install -y python3.7 python3.7-dev python3-pip
python3.7 -m pip install numpy
```

Install apache developer files (OPTIONAL for mod_visus):

```
sudo apt-get install -y libapr1 libapr1-dev libaprutil1 libaprutil1-dev apache2-dev
```

Install qt5 (5.12 or another version):

```
#sudo add-apt-repository -y ppa:beineri/opt-qt592-xenial
#sudo apt update
#sudo apt-get install -y qt59base qt59imageformats

sudo add-apt-repository -y ppa:beineri/opt-qt-5.12.8-xenial
sudo apt update
sudo apt-get install -y qt512base qt512imageformats
```


Compile OpenVisus (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

export Python_EXECUTABLE=python3.7
export Qt5_DIR=/opt/qt512/lib/cmake/Qt5 
alias python=${Python_EXECUTABLE}

mkdir build 
cd build

cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4 
cmake --build ./ --target install --config Release

python -m pip install --upgrade pip

export PYTHONPATH=./Release
python -m OpenVisus configure --user
python -m OpenVisus viewer
```

<!--//////////////////////////////////////////////////////////////////////// -->
## Minimal compilation

Minimal compilation disable image support, network support, Python extensions and supports 
only OpenVisus IDX file format:


```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build
cd build

# on windows
cmake -G "Visual Studio 16 2019" -A "x64" -DVISUS_MINIMAL=1 ../ 
cmake --build . --target ALL_BUILD --config Release

# on apple
cmake -GXcode -DVISUS_MINIMAL=1 ../
cmake --build ./ --target ALL_BUILD --config Release 

# on linux
cmake -DVISUS_MINIMAL=1 ../
cmake --build ./ --target all 
```


<!--//////////////////////////////////////////////////////////////////////// -->
## Commit CI

For OpenVisus developers only:

```
TAG=$(python Libs/swig/setup.py new-tag) && echo ${TAG}
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


