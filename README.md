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
python -c "import OpenVisus"
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

otherwise you can use InternalLibs (slow):

```
set VISUS_INTERNAL_DEFAULT=1
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
set GENERATOR=Visual Studio 15 2017 Win64
set QT5_DIR=C:\Qt\Qt5.9.2\5.9.2\msvc2017_64
set CMAKE="C:\Program Files\CMake\bin\cmake.exe"
set GIT_CMD=C:\Program Files\Git\bin\git.exe
set SWIG_EXECUTABLE=C:\ProgramData\chocolatey\bin\swig.exe
set CONFIGURATION=RelWithDebInfo

IF DEFINED  CMAKE_TOOLCHAIN_FILE (
	%CMAKE% -G "%GENERATOR%" -DQt5_DIR="%QT5_DIR%\lib\cmake\Qt5" -DGIT_CMD="%GIT_CMD%" -DSWIG_EXECUTABLE="%SWIG_EXECUTABLE%" -DCMAKE_TOOLCHAIN_FILE="%CMAKE_TOOLCHAIN_FILE%" -DVCPKG_TARGET_TRIPLET="%VCPKG_TARGET_TRIPLET%"  ..
) ELSE (
	%CMAKE% -G "%GENERATOR%" -DQt5_DIR="%QT5_DIR%\lib\cmake\Qt5" -DGIT_CMD="%GIT_CMD%" -DSWIG_EXECUTABLE="%SWIG_EXECUTABLE%" ..
)
	
%CMAKE% --build . --target ALL_BUILD   --config %CONFIGURATION%
%CMAKE% --build . --target RUN_TESTS   --config %CONFIGURATION%
%CMAKE% --build . --target INSTALL     --config %CONFIGURATION% 
%CMAKE% --build . --target deploy      --config %CONFIGURATION% 
```

To test if visusviewer it's working double click on the file install\visusviewer.bat.



## MacOSX compilation


Install python using pyenv (best to avoid conflicts):

```
cd $HOME     
curl -L https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer -O 
chmod a+x pyenv-installer && ./pyenv-installer  && rm ./pyenv-installer
 
cat<<EOF >> ~/.bashrc
export PATH="\$HOME/.pyenv/bin:\$PATH" 
eval "\$(pyenv init -)"          
eval "\$(pyenv virtualenv-init -)"
EOF
source ~/.bashrc

PYTHON_VERSION=3.6.6 # change it if needed
CONFIGURE_OPTS=--enable-shared pyenv install -s $PYTHON_VERSION    
CONFIGURE_OPTS=--enable-shared pyenv global     $PYTHON_VERSION 
python -m pip install --user --upgrade pip
python -m pip install --user --upgrade numpy 
```


Install prerequisites:

```
# if command line tools do not work, type the following: sudo xcode-select --reset
sudo xcode-select --install

# install brew 
if ! [ -x "$(command -v brew)" ]; then
  ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

# install brew dependencies
brew install git cmake swig qt5  
brew upgrade cmake
python -m pip install --user --upgrade numpy 
```

Choose if  you want to use precompiled brew libraries (fast) or not (slow):

```
VISUS_INTERNAL_DEFAULT=0 # 0 means: use brew

if [ $VISUS_INTERNAL_DEFAULT -eq 0 ]; then
	brew install zlib lz4 tinyxml freeimage openssl curl
fi
```

Then compile OpenVisus:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus 
mkdir build 
cd build

cmake -GXcode \
  -DPYTHON_VERSION=${PYTHON_VERSION} \
  -DPYTHON_EXECUTABLE=$(pyenv prefix)/bin/python \
  -DPYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_VERSION:0:3}m \
  -DPYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_VERSION:0:3}m.dylib \
  -DQt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5 \
  -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
  ..
  
CONFIGURATION=RelWithDebInfo
cmake --build . --target ALL_BUILD   --config $CONFIGURATION -- -jobs 8
cmake --build . --target RUN_TESTS   --config $CONFIGURATION
cmake --build . --target install     --config $CONFIGURATION
cmake --build . --target deploy      --config $CONFIGURATION  
```

To test if it's working:

```
cd install

# OpenVisus embedding python
./visusviewer.command      

# OpenVisus extending python
PYTHONPATH=$(pwd):$(pwd)/bin python -c "import OpenVisus"
```




      
## Linux compilation

Install prerequisites (assuming you are using python 3.x).

For Ubuntu 16.04:

```
sudo apt install -y cmake git build-essential swig libssl-dev uuid-dev python3 python3-pip t5-default qttools5-dev-tools
	
# OPTIONAL (If you want to build Apache plugin)
# sudo apt install -y apache2 apache2-dev 
```
	
For OpenSuse Leap:

```
# OPTIONAL
# sudo zypper refresh && sudo zypper -n update && sudo zypper -n patch     
sudo zypper -n in -t pattern devel_basis cmake cmake-gui git swig curl python3 python3-pip python3-devel libuuid-devel libopenssl-devel glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel 
```

Install numpy and deploy depencencies:

```
python3 -m pip install --user --upgrade pip
python3 -m pip install --user --upgrade numpy 
```

Compile OpenVisus. Decide if you want to use OS libraries (fast) or internal libraries(slow):

```
VISUS_INTERNAL_DEFAULT=0
if [ $VISUS_INTERNAL_DEFAULT -eq 0 ]; then 
  sudo apt-get install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi
```

Then:

```
git clone https://github.com/sci-visus/OpenVisus
cd OpenVisus
mkdir build  && cd build

# if you want to use an IDE
#GENERATOR="CodeBlocks - Unix Makefiles" 
GENERATOR="Unix Makefiles"

CMAKE_BUILD_TYPE=RelWithDebInfo

cmake  -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} -G "$GENERATOR" ../   
cmake --build . --target all      -- -j 8
cmake --build . --target test     
cmake --build . --target install   
cmake --build . --target deploy   
```

To test if it's working:

```
LD_LIBRARY_PATH=$(pwd) PYTHONPATH=$(pwd) ./visusviewer 
LD_LIBRARY_PATH=$(pwd) PYTHONPATH=$(pwd) python3 -c "import OpenVisus"
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

### Ubuntu

Most of the command needs root access:

```
sudo /bin/bash
```

Install apache dependencies and set up where visus will be:

```
export VISUS_HOME=/home/visus
apt-get install -y apache2 apache2-devel
```

Clone and compile visus:

```
git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
cd $VISUS_HOME
mkdir build
cd build
cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
make -j 4
```

Setup apache for mod_visus: 

```
APACHE_EMAIL=youremail@here.com
cat <<EOF > /etc/apache2/sites-enabled/000-default.conf
<VirtualHost *:80>
  ServerAdmin $APACHE_EMAIL
  DocumentRoot /var/www
  <Directory /var/www>
    Options Indexes FollowSymLinks MultiViews
    AllowOverride All
    Order allow,deny
    Allow from all
  </Directory> 
  <Location /mod_visus>
    SetHandler visus
    DirectorySlash Off
    Header set Access-Control-Allow-Origin "*"
  </Location>
</VirtualHost>
EOF

echo "LoadModule visus_module /home/visus/build/libmod_visus.so" > /etc/apache2/mods-available/visus.load
a2enmod visus 
```

Add a dataset:

```
cat <<EOF >  $VISUS_HOME/visus.config
<?xml version="1.0" ?>
<visus>
  <dataset name='cat' url='file://$VISUS_HOME/datasets/cat/visus.idx' permissions='public'/>
</visus>
EOF
```

Finally start apache:

```
chown -R www-data   $VISUS_HOME
chmod -R a+rX    $VISUS_HOME
systemctl stop apache2
rm -f /var/log/apache2/error.log 
systemctl start apache2
more /var/log/apache2/error.log # check logs 
```

If you want to start apache in the foreground:

```
stop apache2
rm -f /usr/local/apache2/logs/httpd.pid
source /etc/apache2/envvars
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
rm -f /var/log/apache2/error.log 
exec /usr/sbin/apache2 -DFOREGROUND
```

To test it, in another terminal:

```
curl -v "http://localhost/mod_visus?action=readdataset&dataset=cat"
```

### OpenSuse

Most of the command needs root access:

```
sudo /bin/bash
```

Install apache dependencies and set up where visus will be:

```
export VISUS_HOME=/home/visus
zypper -n in apache2 apache2-devel
```

Clone and compile visus:

```
git clone https://github.com/sci-visus/OpenVisus $VISUS_HOME
cd $VISUS_HOME
mkdir build
cd build
cmake ../ -DVISUS_GUI=0 -DVISUS_HOME=$VISUS_HOME -DVISUS_PYTHON_SYS_PATH=$(pwd)
make -j 4
```

Setup apache for mod_visus: 

```
APACHE_EMAIL=youremail@here.com
cat <<EOF > /etc/apache2/conf.d/000-default.conf
<VirtualHost *:80>
  ServerAdmin $APACHE_EMAIL
  DocumentRoot /srv/www
  <Directory /srv/www>
    Options Indexes FollowSymLinks MultiViews
    AllowOverride All
    <IfModule !mod_access_compat.c>
      Require all granted
    </IfModule>
    <IfModule mod_access_compat.c>
      Order allow,deny
      Allow from all
    </IfModule>
  </Directory> 
  <Location /mod_visus>
    SetHandler visus
    DirectorySlash Off
  </Location>
</VirtualHost>
EOF

echo "LoadModule visus_module /usr/lib64/apache2-prefork/mod_visus.so" >> /etc/apache2/loadmodule.conf
ln -s $VISUS_HOME/build/libmod_visus.so  /usr/lib64/apache2-prefork/mod_visus.so
a2enmod visus 

cat <<EOF >  $VISUS_HOME/visus.config
<?xml version="1.0" ?>
<visus>
  <dataset name='cat' url='file://$VISUS_HOME/datasets/cat/visus.idx' permissions='public'/>
</visus>
EOF
```

Finally start apache:

```
chown -R wwwrun  $VISUS_HOME
chmod -R a+rX    $VISUS_HOME
systemctl stop apache2
rm -f /var/log/apache2/error_log 
systemctl start apache2
more /var/log/apache2/error_log # see 
```

If you want to start apache in the foreground:

```
systemctl stop apache2
rm -f /run/httpd.pid
mkdir -p /var/log/apache2
rm -f /var/log/apache2/error_log 
/usr/sbin/httpd -DFOREGROUND
```

To test it, in another terminal:

```
curl -v "http://localhost/mod_visus?action=readdataset&dataset=cat"
```

# Auto Deploy	

`.travis.yml` and `.appveyor.ymp` deploy automatically to `GitHub Releases` when the Git commit is tagged.
To properly tag your commit, your first need to edit the CMake/setup.py and change the VERSION number. 
Then tag your code in git:

```
git commit -a -m "...your message here..." 
git config --global push.followTags true 
VERSION=X.Y.Z # replace  with the same numbers from CMake/setup.py
git tag -a "$VERSION" -m "$VERSION" 
git push
```


