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

[Windows compilation](#windows-compilation)

[MacOSX compilation](#macosx-compilation)

[Linux compilation](#linux-compilation)


## PIP distribution

You can install/test OpenVisus in python using Pip:

```
python -m pip install --user numpy OpenVisus==1.2.142
python -m OpenVisus configure 
python -c "import OpenVisus"
cd /path/to/OpenVisus
python Samples/python/Array.py
python Samples/python/Dataflow.py
python Samples/python/Idx.py
python Samples/python/Viewer.py
```

Or you can download OpenVisus from GitHub releases (version 1.2.142), extract it and in Windows:

```
cd \your\OpenVisus\directory
python configure.py
set PYTHONPATH=%cd%;%PYTHONPATH%
python Samples\python\Array.py
python Samples\python\Dataflow.py
python Samples\python\Idx.py
python Samples\python\Viewer.py
```

in Osx/Linux:

```
cd /your/OpenVisus/directory
python configure.py
export PYTHONPATH=$(pwd):${PYTHONPATH}
python Samples/python/Array.py
python Samples/python/Dataflow.py
python Samples/python/Idx.py
python Samples/python/Viewer.py
```


## Windows compilation

Install git, cmake and swig. 
The fastest way is to use `chocolatey` i.e from an Administrator Prompt:

```
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
choco install -y -allow-empty-checksums git cmake swig 
```

Install [Python3.7] (https://www.python.org/ftp/python/3.7.0/python-3.7.0-amd64.exe):

Install [Qt5 5.11](https://download.qt.io/official_releases/qt/5.11/5.11.2/qt-opensource-windows-x86-5.11.2.exe) (the version is important for PyQt5 compatibility)

Install [Microsoft vcpkg](https://github.com/Microsoft/vcpkg) :

```
cd c:\
mkdir tools
cd tools
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
```

Then compile/install OpenVisus:

```
cd OpenVisus

REM *** change path as needed ***
set PYTHON_EXECUTABLE=C:\Python37\python.exe
set CMAKE_EXECUTABLE=C:\Program Files\CMake\bin\cmake.exe
set QT5_DIR=c:\Qt\5.11.2\msvc2015_64
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
CMAKE_BUILD_TYPE=RelWithDebInfo build.sh
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
CMAKE_BUILD_TYPE=RelWithDebInfo ./build.sh  
```

To test if it's working:

```
cd install
./visus.sh
./visusviewer.sh
```



