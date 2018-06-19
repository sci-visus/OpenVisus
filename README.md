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

[Windows compilation](#windows-compilation)

[MacOSX compilation](#macosx-compilation)

[Linux compilation](#linux-compilation)

[Use OpenVisus as submodule](#use-openvisus-as-submodule)

[mod_visus](#mod_visus)
	
## Windows compilation

Install [Python 3.x](https://www.python.org/ftp/python/3.6.3/python-3.6.3-amd64.exe) 
You may want to check "*Download debugging symbols*" and "*Download debugging libraries*" if you are planning to debug your code. 

Install numpy and deploy packages::

	pip3 install numpy setuptools wheel
  
Install PyQt5:

  pip3 install PyQt5==5.9.2
  
Install [Qt5](http://download.qt.io/official_releases/qt/5.9/5.9.2/qt-opensource-windows-x86-5.9.2.exe) 

Install chocolatey. From an Administrator Prompt::

	@"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoProfile -InputFormat None -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))" && SET "PATH=%PATH%;%ALLUSERSPROFILE%\chocolatey\bin"
	choco install -y  -allow-empty-checksums git cmake swig

Install Microsoft *vcpkg*. From an Prompt::

	cd c:\
	mkdir Tools
	cd Tools
	git clone https://github.com/Microsoft/vcpkg
	cd vcpkg
	.\bootstrap-vcpkg.bat
	.\vcpkg install lz4:x64-windows tinyxml:x64-windows zlib:x64-windows openssl:x64-windows curl:x64-windows freeimage:x64-windows
	
Compile OpenVisus. From a prompt::

	cd c:\
	mkdir projects
	cd projects
	git clone https://github.com/sci-visus/OpenVisus
	cd OpenVisus
	mkdir build
	cd build
	set CMAKE="C:\Program Files\CMake\bin\cmake.exe"
	%CMAKE% ^
		-G "Visual Studio 15 2017 Win64" ^
		-DCMAKE_TOOLCHAIN_FILE="c:/Tools/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
		-DVCPKG_TARGET_TRIPLET="x64-windows" ^
		-DQt5_DIR="C:/Qt/Qt5.9.2/5.9.2/msvc2017_64/lib/cmake/Qt5" ^
		-DGIT_CMD="C:\Program Files\Git\bin\git.exe" ^
		-DSWIG_EXECUTABLE="C:\ProgramData\chocolatey\bin\swig.exe" ^
		..
	%CMAKE% --build . --target ALL_BUILD --config Release
	%CMAKE% --build . --target RUN_TESTS --config Release 
	REM OPTIONAL
	REM %CMAKE% --build . --target INSTALL --config Release 

To test if it's working::

	REM change path accordingly
	SET PYTHONPATH=C:\projects\OpenVisus\build\Release
	SET PATH=c:\python36;C:\Qt\Qt5.9.2\5.9.2\msvc2017_64\bin
	.\Release\visusviewer.exe
	REM use python_d.exe if you are using the Debug version
	REM add -vv if you want very verbose output
	c:\Python36\python.exe -c "from visuspy import *; print('visuspy is working')"

## MacOSX compilation

Install brew and OpenVisus prerequisites::

	ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	brew install git cmake swig qt5 lz4 tinyxml zlib openssl curl freeimage python3
	pip3 install numpy setuptools wheel

Run xcode command line tools:

	sudo xcode-select --install
	# if command line tools do not work, type the following:
	# sudo xcode-select --reset


Compile OpenVisus. From a prompt::

	git clone https://github.com/sci-visus/OpenVisus
	cd OpenVisus
	mkdir build
	cd build
	cmake -GXcode -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl -DQt5_DIR=/usr/local/opt/qt/lib/cmake/Qt5 .. 
	cmake --build . --target ALL_BUILD --config Release
	# optional
	# cmake --build . --target RUN_TESTS --config Release 
	# cmake --build . --target install   --config Release

To test if it's working::

	export PYTHONPATH=$(pwd)/Release
	python3 -c "from visuspy import *; print('visuspy is working')"

## Linux compilation

Install prerequisites (assuming you are using python 3.x).

For Ubuntu 16.04::
OpenSUSE Leap::

	sudo apt install -y liblz4-dev libtinyxml-dev cmake git build-essential swig libfreeimage-dev \
		libcurl4-openssl-dev libssl-dev uuid-dev python3 python3-pip \
		qt5-default qttools5-dev-tools

	# If you want to build Apache plugin::
	sudo apt install -y apache2 apache2-dev
	
For OpenSuse Leap::

	sudo zypper refresh      # (OPTIONAL)
	sudo zypper -n update    # (OPTIONAL)
	sudo zypper -n patch     # (OPTIONAL)
	sudo zypper -n in -t pattern devel_basis \
		cmake git swig curl cmake-gui \
		python3 python3-pip python3-devel \
		zlib-devel liblz4-devel libtinyxml-devel libuuid-devel freeimage-devel libcurl-devel libopenssl-devel glu-devel \
		libQt5Concurrent-devel libQt5Network-devel \libQt5Test-devel libQt5OpenGL-devel libQt5PrintSupport-devel

Install numpy and deploy depencencies::

	sudo pip3 install --upgrade pip
	sudo pip3 install numpy setuptools wheel

Compile OpenVisus::

	git clone https://github.com/sci-visus/OpenVisus
	cd OpenVisus
	mkdir build 
	cd build
	cmake ../
	cmake --build . --target all 
	# optional
	# cmake --build . --target test
	# cmake --build . --target install

To test if it's working::

	export LD_LIBRARY_PATH=$(pwd)
	export PYTHONPATH=$(pwd)
	./visusviewer
	python3 -c "from visuspy import *; print('visuspy is working')"
	
## Use OpenVisus as submodule

In your repository::

	git submodule add https://github.com/sci-visus/OpenVisus
	
Create a CMakeLists.txt with the following content::

	CMAKE_MINIMUM_REQUIRED(VERSION 3.1) 

	project(YourProjectName)

	include(OpenVisus/CMake/VisusMacros.cmake)
	SetupCMake()
	add_subdirectory(OpenVisus)
	...your code...
	target_link_libraries(your_executable VisusAppKit) # or whatever you need

	

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

	

