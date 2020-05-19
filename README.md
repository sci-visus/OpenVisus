```
Copyright (c) 2010-2018 ViSUS L.L.C., 
Scientific Computing and Imaging Institute of the University of Utah
 
ViSUS L.L.C., 50 W. Broadway, Ste. 300, 84101-2044 Salt Lake City, UT
University of Utah, 72 S Central Campus Dr, Room 3750, 84112 Salt Lake City, UT
 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

* Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from
this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

For additional information about this project contact: pascucci@acm.org
For support: support@visus.net
```

# ViSUS Visualization project  

![GitHub Actions](https://github.com/sci-visus/OpenVisus/workflows/BuildOpenVisus/badge.svg)


The mission of ViSUS.org is to provide support for the scientific community with Big Data, management, analysis and visualization tools.
In this website we provide access to open source software tools and libraries such as the ViSUS framework and the PIDX library.
These softwares are distributed under the permissive BSD license.

Table of content:

[PIP Distribution](#pip-distribution)

[Conda Distribution](#conda-distribution)

[Windows compilation](#windows-compilation)

[MacOSX compilation](#macosx-compilation)

[Linux compilation](#linux-compilation)

[VisusSlam](#visus-slam)

[Commit CI](#commit-ci)



<!--//////////////////////////////////////////////////////////////////////// -->
## PIP distribution

Type:

```
python -m pip install --no-cache-dir --upgrade --force-reinstall OpenVisus
python -m OpenVisus configure 
python -m OpenVisus test
python -m OpenVisus convert
python -m OpenVisus viewer
```

<!--//////////////////////////////////////////////////////////////////////// -->
## Conda distribution

Type:

```
conda uninstall -y openvisus # optional
conda install -y -c visus openvisus
python -m OpenVisus configure 
python -m OpenVisus test
python -m OpenVisus convert
python -m OpenVisus viewer
```

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

set PYTHON_PATH=.\Release
python -m OpenVisus test
python -m OpenVisus configure
python -m OpenVisus convert
python -m OpenVisus viewer
```

<!--//////////////////////////////////////////////////////////////////////// -->
## MacOSX compilation

Make sure you have command line toos:

```
sudo xcode-select --install || sudo xcode-select --reset
```

Install the following presequisites (for example using brew): 

```
swig cmake python3.x qt5
```

Build the repository (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

# update submodules
git pull --recurse-submodules

Python_ROOT_DIR=$(brew --prefix python3)
Qt5_DIR=$(brew --prefix qt5)/lib/cmake/Qt5
alias python=${Python_ROOT_DIR}/bin/python3

mkdir build 
cd build

cmake -GXcode -DPython_ROOT_DIR=${Python_ROOT_DIR} -DQt5_DIR=${Qt5_DIR} -DVISUS_SLAM=0 ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4 

export PYTHONPATH=$(pwd)/Release
python -m OpenVisus test
python -m OpenVisus configure
python -m OpenVisus convert
python -m OpenVisus viewer
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
cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} -DVISUS_SLAM=0 ../
cmake --build ./ --target all --config Release --parallel 4 

python -m pip install --upgrade pip

export PYTHONPATH=./Release
python -m OpenVisus test
python -m OpenVisus configure
python -m OpenVisus convert
python -m OpenVisus viewer
```

<!--//////////////////////////////////////////////////////////////////////// -->
##  VisusSlam

On osx install prerequisites:

``` 
brew install exiftool zbar 
```

Install python dependencies:

```
python -m pip install --upgrade pip
python -m pip install matplotlib pymap3d pytz pyzbar scikit-image scipy pysolar json-tricks cmapy tifffile pyexiftool opencv-python opencv-contrib-python
```

Then install OpenVisus slam package:

``` 
python -m pip install --no-cache-dir --upgrade --force-reinstall OpenVisus

python -m OpenVisus configure
python -m OpenVisus test
   
# ON MACOS ONLY, you may need to solve conflicts between Qt embedded in opencv2 and PyQt5 we are going to use:
python -m pip uninstall -y opencv-python          opencv-contrib-python
python -m pip install      opencv-python-headless opencv-contrib-python-headless 

python -m OpenVisus slam
```


<!--//////////////////////////////////////////////////////////////////////// -->
## Commit CI

Type:

```
TAG=$(python Libs/Kernel/setup.py new-tag) && echo ${TAG}
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


