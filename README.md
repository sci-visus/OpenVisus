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


* `osx linux` build status: [![Build Status](https://travis-ci.com/sci-visus/visus.svg?token=yzpwCyVPupwSzFjgTCoA&branch=master)](https://travis-ci.com/sci-visus/visus)

* `windows` build status: [![Windows Build status](https://ci.appveyor.com/api/projects/status/32r7s2skrgm9ubva/branch/master?svg=true)](https://ci.appveyor.com/api/projects/status/32r7s2skrgm9ubva/branch/master)

Table of content:


[PIP Distribution](#pip-distribution)

[GitHub Releases Distribution](#github-releases-distribution)

[Windows compilation](#windows-compilation)

[MacOSX compilation](#macosx-compilation)

[Linux compilation](#linux-compilation)


## PIP distribution

You can install/test OpenVisus in python using Pip.
Under Windows:

```
REM *** change PYTHON_EXECUTABLE  as needed      *** 
REM *** only python 3.6/3.7 64 bit are supported ***
set PYTHON_EXECUTABLE=c:\Python37\python.exe

%PYTHON_EXECUTABLE% -m pip install pip --upgrade
%PYTHON_EXECUTABLE% -m pip uninstall OpenVisus
%PYTHON_EXECUTABLE% -m pip install --user numpy OpenVisus==1.2.182


REM *** Type this command: %PYTHON_EXECUTABLE%  -m OpenVisus dirname ***
REM *** and replace the following path with the one you just got     ***
cd "C:\Users\%USERNAME%\AppData\Roaming\..."

REM *** execute the following step to finilize the installation                                                                   ***
REM *** add --use-pyqt if you want to use PyQt5 instead of C++ Qt5 (needed if you are going to mix Python and C++ Gui components) ***
%PYTHON_EXECUTABLE% .\configure.py

.\visusviewer.bat

%PYTHON_EXECUTABLE% Samples/python/Array.py
%PYTHON_EXECUTABLE% Samples/python/Dataflow.py
%PYTHON_EXECUTABLE% Samples/python/Idx.py
```

On Linux/Osx make sure you are using the right python binary. 
For example if you are using pyenv:

```
export PATH="$HOME/.pyenv/bin:$PATH"
eval "$(pyenv init -)"
eval "$(pyenv virtualenv-init -)"
pyenv global <type_your_version_here>
python --version # check the version here
```

Then on Linux/osx:

```
python -m pip install pip --upgrade
python -m pip uninstall -y OpenVisus
python -m pip install --user numpy OpenVisus==1.2.182
cd $(python -m OpenVisus dirname)


# finilize installation
# set PYQT=1 to use PyQt5 instead of C++ Qt5 (needed if you are going to mix Python and C++ Gui components)
PYQT=0
if (( PYQT == 1 )) ; then
	python ./configure.py --use-pyqt
else
	python ./configure.py
fi

python Samples/python/Array.py
python Samples/python/Dataflow.py
python Samples/python/Idx.py
python Samples/python/Viewer.py
```

Then on Linux type './visusviewer.sh', on osx `./visusviewer.command`.

## GitHub Releases distribution

You can download OpenVisus from GitHub releases (use the same version). Unzip/Untar your file. 
And in Windows:

```
cd \your\OpenVisus\directory

REM *** change PYTHON_EXECUTABLE  as needed      *** 
REM *** only python 3.6/3.7 64 bit are supported ***
set PYTHON_EXECUTABLE=c:\Python37\python.exe

REM add --use-pyqt if you want to use PyQt5 instead of C++ Qt5 (needed if you are going to mix Python and C++ Gui components)
%PYTHON_EXECUTABLE% .\configure.py

set PYTHONPATH=%cd%;%PYTHONPATH%

.\visusviewer.bat

%PYTHON_EXECUTABLE% Samples\python\Array.py
%PYTHON_EXECUTABLE% Samples\python\Dataflow.py
%PYTHON_EXECUTABLE% Samples\python\Idx.py
```

in Osx/Linux:

```
cd /your/OpenVisus/directory

# add --use-pyqt if you want to use PyQt5 instead of C++ Qt5 (needed if you are going to mix Python and C++ Gui components)
python ./configure.py

export PYTHONPATH=$(pwd):${PYTHONPATH}

python Samples/python/Array.py
python Samples/python/Dataflow.py
python Samples/python/Idx.py
```

## Windows compilation

Install git, cmake and swig. 
The fastest way is to use `chocolatey` i.e from an Administrator Prompt:

```
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
choco install -y -allow-empty-checksums git cmake swig 
```

Install [Python3.7] (https://www.python.org/ftp/python/3.7.0/python-3.7.0-amd64.exe)
Install [Qt5](http://download.qt.io/official_releases/online_installers/qt-unified-windows-x86-online.exe) 

If you want to use Microsoft vcpkg (faster) install [vcpkg](https://github.com/Microsoft/vcpkg):

```
cd c:\
mkdir tools
cd tools
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
set CMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
set VCPKG_TARGET_TRIPLET=x64-windows
```

To compile OpenVIsus:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

REM *** change path as needed *** 
set PYTHON_EXECUTABLE=C:\Python37\python.exe
set CMAKE_EXECUTABLE=C:\Program Files\CMake\bin\cmake.exe
set QT5_DIR=c:\Qt\5.11.2\msvc2015_64\lib\cmake\Qt5
.\build.bat
```

To test if it's working:

```
cd install
.\visus.bat
.\visusviewer.bat 
```


## MacOSX compilation

Make sure you have command line toos:

```
sudo xcode-select --install
# if command line tools do not work, type the following: sudo xcode-select --reset
```

Build the repository:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
build.sh
```

To test if it's working:

```
cd install
./visus.command
./visusviewer.command 
```
      
## Linux compilation

Build the repository:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
./build.sh  
```

To test if it's working:

```
cd install
./visus.sh
./visusviewer.sh
```

Note that on linux you can even compile using docker. An example is:


```
BUILD_DIR=$(pwd)/build/docker/manylinux PYTHON_VERSION=3.6.1 VISUS_INTERNAL_DEFAULT=1 DOCKER_IMAGE=quay.io/pypa/manylinux1_x86_64 ./build.sh
BUILD_DIR=$(pwd)/build/docker/trusty    PYTHON_VERSION=3.6.1 VISUS_INTERNAL_DEFAULT=1 DOCKER_IMAGE=ubuntu:trusty                  ./build.sh
BUILD_DIR=$(pwd)/build/docker/bionic    PYTHON_VERSION=3.6.1 VISUS_INTERNAL_DEFAULT=1 DOCKER_IMAGE=ubuntu:bionic                  ./build.sh
BUILD_DIR=$(pwd)/build/docker/xenial    PYTHON_VERSION=3.6.1 VISUS_INTERNAL_DEFAULT=1 DOCKER_IMAGE=ubuntu:xenial                  ./build.sh
BUILD_DIR=$(pwd)/build/docker/leap      PYTHON_VERSION=3.6.1 VISUS_INTERNAL_DEFAULT=1 DOCKER_IMAGE=opensuse:leap                  ./build.sh
```


