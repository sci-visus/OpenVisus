#!/bin/bash


# //////////////////////////////////////////////////////
function PushCmakeOption {
	if [ -n "$2" ] ; then
		cmake_opts+=" -D$1=$2"
	fi
}

# //////////////////////////////////////////////////////
function SetupOpenVisusCMakeOptions {

	cmake_opts=""
	PushCmakeOption PYTHON_VERSION         ${PYTHON_VERSION}
	PushCmakeOption CMAKE_BUILD_TYPE       ${CMAKE_BUILD_TYPE}
	PushCmakeOption VISUS_INTERNAL_DEFAULT ${VISUS_INTERNAL_DEFAULT}
	PushCmakeOption DISABLE_OPENMP         ${DISABLE_OPENMP}
	PushCmakeOption VISUS_GUI              ${VISUS_GUI}
	PushCmakeOption OPENSSL_ROOT_DIR       ${OPENSSL_ROOT_DIR}
	PushCmakeOption PYTHON_EXECUTABLE      ${PYTHON_EXECUTABLE}
	PushCmakeOption PYTHON_INCLUDE_DIR     ${PYTHON_INCLUDE_DIR}
	PushCmakeOption PYTHON_LIBRARY         ${PYTHON_LIBRARY}
	PushCmakeOption Qt5_DIR                ${Qt5_DIR}
	PushCmakeOption SWIG_EXECUTABLE        ${SWIG_EXECUTABLE}
	PushCmakeOption VISUS_HOME             ${VISUS_HOME}
	PushCmakeOption CMAKE_INSTALL_PREFIX   ${CMAKE_INSTALL_PREFIX}
	PushCmakeArg    VISUS_PYTHON_SYS_PATH  ${VISUS_PYTHON_SYS_PATH}	
	PushCmakeOption PYPI_USERNAME          ${PYPI_USERNAME}
	PushCmakeOption PYPI_PASSWORD          ${PYPI_PASSWORD}
}



# //////////////////////////////////////////////////////
function BuildOpenVisus {

	mkdir -p build
	cd build

	cmake ${cmake_opts} ../ 
	
	cmake --build . --target all -- -j 4
	cmake --build . --target test
	cmake --build . --target install 
	cmake --build . --target deploy 
	cmake --build . --target bdist_wheel
	cmake --build . --target sdist 
	
	if ((DEPLOY_PYPI==1)); then 
	  cmake --build . --target pypi 
	fi

}

# //////////////////////////////////////////////////////
function DownloadFile {
   curl -L --insecure "$1" -O
}


# //////////////////////////////////////////////////////
# NOTE for linux: mixing python openssl and OpenVisus internal openssl cause crashes so I'm always using this one
function InstallOpenSSL {

  if [ ! -f $DEPS_INSTALL_DIR/openssl/lib/libssl.a ]; then
    echo "Compiling openssl"
    DownloadFile "https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
    tar xvzf openssl-1.0.2a.tar.gz 
    pushd openssl-1.0.2a 
    ./config -fpic shared --prefix=$DEPS_INSTALL_DIR/openssl
    make 
    make install 
    popd
    rm -Rf openssl-1.0.2a*
  fi
  
  export OPENSSL_ROOT_DIR=$DEPS_INSTALL_DIR/openssl	
  export OPENSSL_INCLUDE_DIR=${OPENSSL_ROOT_DIR}/include
  export OPENSSL_LIB_DIR=${OPENSSL_ROOT_DIR}/lib
  export LD_LIBRARY_PATH=${OPENSSL_LIB_DIR}:$LD_LIBRARY_PATH
}

# //////////////////////////////////////////////////////
function InstallPatchElf {

	# already exists?
	if ! [ -x "$(command -v patchelf)" ]; then
		return
	fi

	if [ ! -f $DEPS_INSTALL_DIR/patchelf/bin/patchelf ]; then
    echo "Compiling patchelf"
		DownloadFile https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz 
		tar xvzf patchelf-0.9.tar.gz
		pushd patchelf-0.9
		./configure --prefix=$DEPS_INSTALL_DIR/patchelf
		make 
		make install
		popd
		rm -Rf pushd patchelf-0.9*
	fi
	
	export PATH=$DEPS_INSTALL_DIR/patchelf/bin:$PATH
}

# //////////////////////////////////////////////////////
function InstallPython {

  if ! [ -x "$(command -v pyenv)" ]; then
    DownloadFile "https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer"
    chmod a+x pyenv-installer 
    ./pyenv-installer 
    rm -f pyenv-installer 
  fi
  
  export PATH="$HOME/.pyenv/bin:$PATH"
  eval "$(pyenv init -)"
  eval "$(pyenv virtualenv-init -)"
  
  if [ -n "${OPENSSL_INCLUDE_DIR}" ]; then
    CONFIGURE_OPTS=--enable-shared CFLAGS=-I${OPENSSL_INCLUDE_DIR} CPPFLAGS=-I${OPENSSL_INCLUDE_DIR}/ LDFLAGS=-L${OPENSSL_LIB_DIR} pyenv install --skip-existing  ${PYTHON_VERSION}  
  else
    CONFIGURE_OPTS=--enable-shared pyenv install --skip-existing ${PYTHON_VERSION}  
  fi

  pyenv global ${PYTHON_VERSION}  
  pyenv rehash
  python -m pip install --upgrade pip  
  python -m pip install numpy setuptools wheel twine auditwheel 
  
  if [ "${PYTHON_VERSION:0:1}" -gt "2" ]; then
    PYTHON_M_VERSION=${PYTHON_VERSION:0:3}m 
  else
    PYTHON_M_VERSION=${PYTHON_VERSION:0:3}
  fi	
  
  export PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python 
  export PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION} 
  export PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.so
}

# //////////////////////////////////////////////////////
function InstallSwig {
  if [ ! -f $DEPS_INSTALL_DIR/swig/bin/swig ]; then
    DownloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"  
    tar xvzf swig-3.0.12.tar.gz 
    pushd swig-3.0.12 
    DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
    ./Tools/pcre-build.sh 
    ./configure --prefix=$DEPS_INSTALL_DIR/swig
    make -j 4 
    make install 
    popd
    rm -Rf swig-3.0.12*
  fi 
  export PATH=$DEPS_INSTALL_DIR/swig/bin:${PATH} 
}


# //////////////////////////////////////////////////////
function InstallCMake {

	# already exists?
	if [ -x "$(command -v cmake)" ] ; then
		CMAKE_VERSION=$(cmake --version | cut -d' ' -f3)
		if [[ ${CMAKE_VERSION:0:1} > =3 ]]; then
			return
		fi	
	fi
	
  if ! [ -x "$DEPS_INSTALL_DIR/cmake/bin/cmake" ]; then
    echo "Downloading precompiled cmake"
    DownloadFile "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
    tar xvzf cmake-3.4.3-Linux-x86_64.tar.gz
    mv cmake-3.4.3-Linux-x86_64 $DEPS_INSTALL_DIR/cmake
    rm -Rf cmake-3.4.3-Linux-x86_64*
  fi
  
  export PATH=$DEPS_INSTALL_DIR/cmake/bin:${PATH} 
}

# //////////////////////////////////////////////////////
function InstallQtForUbuntu {
	sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y; 
	sudo apt-get update -qq
	sudo apt-get install -qq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt59base
	set +e # temporary disable exit
	source /opt/qt59/bin/qt59-env.sh 
	set -e 
}

# //////////////////////////////////////////////////////
function InstallQtForCentos5 {

	# broken right now
  yum install -y mesa-libGL mesa-libGLU mesa-libGL-devel mesa-libGLU-devel 
  
  DownloadFile "http://xcb.freedesktop.org/dist/xcb-proto-1.11.tar.gz" 
  tar -xzf xcb-proto-1.11.tar.gz 
  pushd xcb-proto-1.11
  ./configure 
  make 
  make install 
  popd   
  
  DownloadFile "http://xcb.freedesktop.org/dist/libpthread-stubs-0.3.tar.gz"
  tar -xzf libpthread-stubs-0.3.tar.gz 
  pushd libpthread-stubs-0.3 
  ./configure 
  make -j8 
  make install
  popd 	
  
  DownloadFile "http://xcb.freedesktop.org/dist/libxcb-1.11.tar.gz"
  tar -xzf libxcb-1.11.tar.gz 
  pushd libxcb-1.11 
  ./configure 
  make -j8 
  make install 
  popd 	
  
  QT_VERSION=5.4.0
  DownloadFile "http://qt.mirror.constant.com/archive/qt/${QT_VERSION:0:3}/${QT_VERSION}/single/qt-everywhere-opensource-src-${QT_VERSION}.tar.gz"
  
  tar -xzf qt-everywhere-opensource-src-${QT_VERSION}.tar.gz 
  
  pushd qt-everywhere-opensource-src-${QT_VERSION}
  
  sed -i "s/#define QTESTLIB_USE_PERF_EVENTS/#undef QTESTLIB_USE_PERF_EVENTS/g" qtbase/src/testlib/qbenchmark_p.h 
  
  ./configure --prefix=$DEPS_INSTALL_DIR -R \\\$$ORIGIN \
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
  popd
  Qt5_DIR=$DEPS_INSTALL_DIR/Qt-${QT_VERSION}/lib/cmake/Qt5/
}



# ///////////////////////////////////////////////////////////
function InstallModVisus {

	VISUS_HOME=$1
	VISUS_DATASETS=$2

	sudo cat << EOF >/etc/apache2/sites-enabled/000-default.conf
<VirtualHost *:80>
  ServerAdmin scrgiorgio@gmail.com
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
	
	sudo cat << EOF >/usr/local/bin/httpd-foreground.sh
#!/bin/bash
set -e 
rm -f /usr/local/apache2/logs/httpd.pid
source /etc/apache2/envvars
mkdir -p $APACHE_RUN_DIR $APACHE_LOCK_DIR $APACHE_LOG_DIR
rm -f /var/log/apache2/error.log 
exec /usr/sbin/apache2 -DFOREGROUND
EOF	
	
	echo "LoadModule visus_module ${VISUS_HOME}/bin/libmod_visus.so" > /etc/apache2/mods-available/visus.load
	a2enmod headers 
	a2enmod visus 
	chmod a+x /usr/local/bin/httpd-foreground.sh 
	
	# fix LD_LIBRARY_PATH problem
	echo "$VISUS_HOME/bin" >> /etc/ld.so.conf 
	ldconfig
	
	echo "<include url='$VISUS_DATASETS/visus.config' />" > $VISUS_HOME/visus.config
	chown -R wwwrun  ${VISUS_HOME}
	chmod -R a+rX    ${VISUS_HOME}
}