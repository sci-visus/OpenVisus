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

[Use OpenVisus as submodule](#use-openvisus-as-submodule)

[mod_visus](#mod_visus)

[Auto deploy] (#auto_deploy)
	
  
  
## PIP distribution

You can install OpenVisus in python using Pip:

in windows:

```
python -m pip install --user numpy OpenVisus
```

in osx,linux:

```
python -m pip install  --user numpy OpenVisus
```

And test it using the following command. 

```
python -c "import VisusKernelPy"
```


## Windows compilation

Install git, cmake and swig. 
The fastest way is to use `chocolatey` i.e from an Administrator Prompt:

```
@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
choco install -y -allow-empty-checksums git cmake swig 
```

Install [Python3.7] (https://www.python.org/ftp/python/3.7.0/python-3.7.0-amd64.exe)

Make sure you have num python installed:

```
REM change path as needed
c:\Python37\python.exe -m pip install --user numpy
```

Install [Qt5](http://download.qt.io/official_releases/qt/5.9/5.9.2/qt-opensource-windows-x86-5.9.2.exe) 


if you want to use [Microsoft vcpkg](https://github.com/Microsoft/vcpkg) (faster):

```
cd c:\
mkdir tools
cd tools
git clone https://github.com/Microsoft/vcpkg
cd vcpkg
.\bootstrap-vcpkg.bat
vcpkg.exe install zlib:x64-windows lz4:x64-windows tinyxml:x64-windows freeimage:x64-windows openssl:x64-windows curl:x64-windows
set CMAKE_TOOLCHAIN_FILE=c:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake
set VCPKG_TARGET_TRIPLET=x64-windows
```

Then:

```
cd c:\
mkdir projects
cd projects
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus

mkdir build
cd build

REM *** change as needed *** 
set PYTHON_EXECUTABLE=C:\Python37\python.exe
set CMAKE_EXECUTABLE=C:\Program Files\CMake\bin\cmake.exe
set QT5_DIR=c:\Qt\5.11.2\msvc2015_64
CMake\build.bat
```

To test if visusviewer it's working double click on the file install\visusviewer.bat.


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
CMAKE_BUILD_TYPE=RelWithDebInfo CMake/build.sh
```

To test if it's working:

```
cd install

# (embedding mode)
bin/visus.app/Contents/MacOS/visus  
bin/visusviewer.app/Contents/MacOS/visusviewer 

# (extending mode)
PYTHONPATH=$(pwd) python -c "import VisusKernelPy"
PYTHONPATH=$(pwd) QT_PLUGIN_PATH=$(pwd)/bin/Qt/plugins python -c "import VisusKernelPy; import VisusGuiPy"
```
      
## Linux compilation


Build the repository:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
CMAKE_BUILD_TYPE=RelWithDebInfo ./CMake/build.sh  
```

To test if it's working (consider you should modify the pyenv part):

```
cd install

# example of embedding mode
LD_LIBRARY_PATH=$(pwd):$(pyenv prefix)/lib PYTHONPATH=$(pwd) bin/visus

# example of extending mode
LD_LIBRARY_PATH=$(pwd)                     PYTHONPATH=$(pwd) python -c "import VisusKernelPy"
```

  
## Use OpenVisus as submodule

In your repository:

```
git submodule add https://github.com/sci-visus/OpenVisus
```
	
Create a CMakeLists.txt with the following content:

```
CMAKE_MINIMUM_REQUIRED(VERSION 3.1) 

project(YourProjectName)

include(OpenVisus/CMake/VisusMacros.cmake)
SetupCMake()
add_subdirectory(OpenVisus)
...your code...
target_link_libraries(your_executable VisusAppKit) # or whatever you need
```
	
## mod_visus

See Docker directory

# Auto Deploy	

`.travis.yml` and `.appveyor.ymp` deploy automatically to `GitHub Releases` when the Git commit is tagged.
Then tag your code in git:

```
vi CMake/setup.py
# .... change the VERSION number in the file...
git commit -a -m "...your message here..." 
git config --global push.followTags true 
 # replace  with the same numbers from CMake/setup.py
VERSION=X.Y.Z git tag -a "$VERSION" -m "$VERSION" 
git push
```


