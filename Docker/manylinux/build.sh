#!/bin/bash

# stop on errors printout commands
set -ex 

if [ -z "$PYTHON_VERSION" ]; then
	PYTHON_VERSION=3.6.6
fi

if [ -z "$CMAKE_BUILD_TYPE" ]; then
	CMAKE_BUILD_TYPE=RelWithDebugInfo
fi

if [ -z "$VISUS_INTERNAL_DEFAULT" ]; then
	VISUS_INTERNAL_DEFAULT=1
fi

if [ -z "$DISABLE_OPENMP" ]; then
	DISABLE_OPENMP=1
fi

if [ -z "$VISUS_GUI" ]; then
	VISUS_GUI=0
fi

# minimal stuff
yum update 
yum install -y zlib-devel curl 

# downloadFile
function downloadFile {
   curl --insecure "$1" -O
}

# install openssl (sudo needed)
# NOTE for linux: mixing python openssl and OpenVisus internal openssl cause crashes
#                 so I'm always using this one
function installOpenSSL {
	downloadFile "https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
	tar xvzf openssl-1.0.2a.tar.gz 
	cd openssl-1.0.2a 
	./config -fpic shared
	make 
	make install 
  echo "/usr/local/ssl/lib" >> /etc/ld.so.conf 
  ldconfig 
	OPENSSL_ROOT_DIR=/usr/local/ssl	
}
 

# install local Python using pyenv
function installPython {

	if ! [ -x "$(command -v pyenv)" ]; then
	
		downloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
		chmod a+x pyenv-installer 
		./pyenv-installer 
		
		cat << EOF >> ~/.bashrc
echo "Activing pyenv"
export PATH="\$HOME/.pyenv/bin:\$PATH"
eval "\$(pyenv init -)"
eval "\$(pyenv virtualenv-init -)"
EOF
	
		source ~/.bashrc	
	
	fi

	CONFIGURE_OPTS=--enable-shared pyenv install ${PYTHON_VERSION}  
	pyenv global ${PYTHON_VERSION}  
	pyenv rehash
	python -m pip install --upgrade pip  
	python -m pip install numpy setuptools wheel twine auditwheel 
	
	if [ "${PYTHON_VERSION:0:1}" -gt "2" ]; then
		PYTHON_M_VERSION=${PYTHON_VERSION:0:3}m 
	else
		PYTHON_M_VERSION=${PYTHON_VERSION:0:3}
	fi	

  PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python 
  PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION} 
  PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
}


# install cmake (sudo needed)
function installCMake {
	downloadFile "https://cmake.org/files/v3.12/cmake-3.12.0-rc1.tar.gz"
	tar xvzf cmake-3.12.0-rc1.tar.gz 
	cd cmake-3.12.0-rc1 
	./bootstrap 
	make -j 4 
	make install 
	hash -r 
	cd .. 
}

# install swig (sudo needed)
function installSwig {
	downloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"  
	tar xvzf swig-3.0.12.tar.gz 
	cd swig-3.0.12 
	downloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
	./Tools/pcre-build.sh 
	./configure 
	make -j 4 
 	make install 
	cd ../
}

# install opengl (Qt dependency)
function installOpenGL {
	yum install -y mesa-libGL mesa-libGLU mesa-libGL-devel mesa-libGLU-devel 
}

# install xcb-proto (Qt dependency)
function installXcbProto {
	downloadFile "http://xcb.freedesktop.org/dist/xcb-proto-1.11.tar.gz" 
	tar -xzf xcb-proto-1.11.tar.gz 
	cd xcb-proto-1.11
	./configure 
	make 
	make install 
	cd .. 
}

# install libpthread-stubs (Qt dependency)
function installPThreadStubs {
	downloadFile "http://xcb.freedesktop.org/dist/libpthread-stubs-0.3.tar.gz"
	tar -xzf libpthread-stubs-0.3.tar.gz 
	cd libpthread-stubs-0.3 
	./configure 
	make -j8 
	make install
	cd .. 
}
		
# installXcb (Qt dependency)
function installXcb {
	downloadFile "http://xcb.freedesktop.org/dist/libxcb-1.11.tar.gz"
	tar -xzf libxcb-1.11.tar.gz 
	cd libxcb-1.11 
	./configure 
	make -j8 
	make install 
	cd .. 
}

# installQt
function installQt {

	QT_VERSION=5.4.0
	downloadFile "http://qt.mirror.constant.com/archive/qt/${QT_VERSION:0:3}/${QT_VERSION}/single/qt-everywhere-opensource-src-${QT_VERSION}.tar.gz"
	
	tar -xzf qt-everywhere-opensource-src-${QT_VERSION}.tar.gz 
	
	cd qt-everywhere-opensource-src-${QT_VERSION}
	
	sed -i "s/#define QTESTLIB_USE_PERF_EVENTS/#undef QTESTLIB_USE_PERF_EVENTS/g" qtbase/src/testlib/qbenchmark_p.h 
	
	./configure -R ‘\\\$$ORIGIN’ \
		-D _X_INLINE=inline \
		-D XK_dead_currency=0xfe6f \
		-D XK_ISO_Level5_Lock=0xfe13 \
		-D FC_WEIGHT_EXTRABLACK=215 \
		-D FC_WEIGHT_ULTRABLACK=FC_WEIGHT_EXTRABLACK \
		-DGLX_GLXEXT_LEGACY \
		-v -opensource \
		-confirm-license \
		-sysconfdir /etc/xdg \
		-release -shared \
		-qt-zlib \
		-qt-libpng \
		-qt-libjpeg \
		-qt-pcre \
		-qt-xcb\
		-qt-xkbcommon \
		-xkb-config-root /usr/share/X11/xkb \
		-no-xcb-xlib \
		-c++11 \
		-nomake examples \
		-nomake tests \
		-no-dbus \
		-no-icu \
		-skip activeqt \
		-skip androidextras \
		-skip connectivity \
		-skip enginio \
		-skip location \
		-skip macextras \
		-skip multimedia \
		-skip quick1 \
		-skip sensors \
		-skip serialport \
		-skip wayland \
		-skip webchannel \
		-skip webengine \
		-skip webkit \
		-skip webkit-examples 
		-skip websockets \
		-skip winextras \
		-skip x11extras 
		
	make 
	make install
	cd ..
	Qt5_DIR=/usr/local/Qt-${QT_VERSION}/lib/cmake/Qt5/
}

installOpenSSL
installPython
installCMake
installSwig

if [ "$VISUS_GUI" -eq "1" ]; then
	installOpenGL
	installXcbProto
	installPThreadStubs
	installXcb
	installQt
fi

mkdir build 
cd build 

opt=""
opt+=" -DPYTHON_VERSION=${PYTHON_VERSION}"
opt+=" -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}"
opt+=" -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT}"
opt+=" -DDISABLE_OPENMP=${DISABLE_OPENMP}"
opt+=" -DVISUS_GUI=${VISUS_GUI}" 
opt+=" -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}"
opt+=" -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}"
opt+=" -DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR}"
opt+=" -DPYTHON_LIBRARY=${PYTHON_LIBRARY}"
opt+=" -DOPENSSL_ROOT_DIR=${OPENSSL_ROOT_DIR}"

if [ "$VISUS_GUI" -eq "1" ]; then
	opt+=" -DQt5_DIR=${Qt5_DIR}"
fi

cmake ${opt} ../ 

cmake --build . --target all -j 4
cmake --build . --target install  
cmake --build . --target deploy   

cd install
python setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=linux_x86_64
WHEEL=$(find ./dist -iname "*.whl") 
mv ${WHEEL} ${WHEEL/linux_x86_64/manylinux1_x86_64}
WHEEL=$(find ./dist -iname "*.whl") 
echo "Created ${WHEEL} "
auditwheel show ${WHEEL} 
	
  










  
