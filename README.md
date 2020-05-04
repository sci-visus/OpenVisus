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

* `Windows` build status: [![Windows Build status](https://ci.appveyor.com/api/projects/status/32r7s2skrgm9ubva/branch/master?svg=true)](https://ci.appveyor.com/api/projects/status/32r7s2skrgm9ubva/branch/master)

* `Apple`   build status: [![APPLE Build Status  ](https://travis-ci.com/sci-visus/visus.svg?token=yzpwCyVPupwSzFjgTCoA&branch=master)](https://travis-ci.com/sci-visus/visus)

* `Linux`   build status [![Linux Build Status   ](https://circleci.com/gh/sci-visus/OpenVisus.svg?style=svg)](<LINK>)

Table of content:


[PIP Distribution](#pip-distribution)

[Conda Distribution](#conda-distribution)

[Windows compilation](#windows-compilation)

[MacOSX compilation](#macosx-compilation)

[Linux compilation](#linux-compilation)

## PIP distribution

Type:

```
python -m pip install numpy OpenVisus
python -m OpenVisus configure 
python -m OpenVisus test
python -m OpenVisus convert
python -m OpenVisus viewer
```


## Conda distribution

Type:

```
conda install numpy
conda install -c visus openvisus
python -m OpenVisus configure 
python -m OpenVisus test
python -m OpenVisus convert
python -m OpenVisus viewer
```


## Windows compilation

Install git, cmake and swig.  The fastest way is to use `chocolatey` (example: `choco install -y git cmake swig`).

Install Python3.x.

Install Qt5 (http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe)

To compile OpenVisus (change as needed):

```
python -m pip install numpy

git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build
cd build
cmake -G "Visual Studio 16 2019" -A "x64" -DQt5_DIR="<FillHere>" -DPython_EXECUTABLE=<FillHere> ../
cmake --build . --target ALL_BUILD --config Release
cmake --build . --target INSTALL   --config Release
set PYTHON_PATH=.\Release
python -m OpenVisus test
python -m OpenVisus convert
python -m OpenVisus viewer
```


## MacOSX compilation

Make sure you have command line toos (`sudo xcode-select --install || sudo xcode-select --reset`):

Install the following presequisites (for example using brew): `swig cmake python3.x qt5`.

Build the repository (change as needed):

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build && cd build
cmake -GXcode -DPython_EXECUTABLE=<string value here> -DQt5_DIR=<string value here> ../
cmake --build ./ --target ALL_BUILD --config Release 
cmake --build ./ --target install --config Release
PYTHONPATH=./Release  python -m OpenVisus test
PYTHONPATH=./Release  python -m OpenVisus convert
PYTHONPATH=./Release  python -m OpenVisus viewer
```
      
## Linux compilation

We are showing as an example how to build OpenVisus on Ubuntu 16.

Install prerequisites:

```
sudo apt install -y patchelf swig
```

Install a recent cmake. For example

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

Install qt5 (note: we are using most of the time the version 5.9 because it's the only one compatible with conda/pyqt):

```
sudo add-apt-repository -y ppa:beineri/opt-qt592-xenial
sudo apt update
sudo apt-get install -Y qt59base qt59imageformats
```


Compile OpenVisus:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build && cd build
cmake -DPython_EXECUTABLE=python3.7 -DQt5_DIR=/opt/qt59/lib/cmake/Qt5 ../
cmake --build ./ --target all     --config Release
cmake --build ./ --target install --config Release
PYTHONPATH=./Release python3.7 -m OpenVisus test
PYTHONPATH=./Release python3.7 -m OpenVisus convert
PYTHONPATH=./Release python3.7 -m OpenVisus viewer
```

## Commit, Continuous Integration deploy

Edit the file `setup.py` and increase the Tag version number.
Commit, tag the current commit and push to origin:

```
TAG=...insert your tag number here...
git commit -a -m "New tag" && git tag -a $TAG -m "$TAG" && git push origin $TAG && git push origin
```


