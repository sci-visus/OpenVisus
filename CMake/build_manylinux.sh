#!/bin/bash

PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}

# override
VISUS_INTERNAL_DEFAULT=1
DISABLE_OPENMP=1
VISUS_GUI=0 
PYPI_PLAT_NAME=manylinux1_x86_64

source "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

CACHED_DIR=~/.cached/OpenVisus
mkdir -p ${CACHED_DIR}
export PATH=${CACHED_DIR}/bin:$PATH


# //////////////////////////////////////////////////////
function InstallCMake {

	if ! [ -x "${CACHED_DIR}/bin/cmake" ]; then
      echo "Downloading precompiled cmake"
      DownloadFile "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
      tar xzf cmake-3.4.3-Linux-x86_64.tar.gz  -C ${CACHED_DIR} --strip-components=1
	fi
}


# //////////////////////////////////////////////////////
# NOTE for linux: mixing python openssl and OpenVisus internal openssl cause crashes so I'm always using this one
function InstallOpenSSL {

	if [ ! -x ${CACHED_DIR}/bin/openssl ]; then
      echo "Compiling openssl"
      DownloadFile "https://www.openssl.org/source/openssl-1.0.2a.tar.gz"
      tar xzf openssl-1.0.2a.tar.gz 
      pushd openssl-1.0.2a 
      ./config -fpic shared --prefix=${CACHED_DIR}
      make -s 
      make install	
      popd
	fi
	
	export OPENSSL_ROOT_DIR=${CACHED_DIR}
	export OPENSSL_INCLUDE_DIR=${OPENSSL_ROOT_DIR}/include 
	export OPENSSL_LIB_DIR=${OPENSSL_ROOT_DIR}/lib
	export LD_LIBRARY_PATH=${OPENSSL_LIB_DIR}:$LD_LIBRARY_PATH
}


# //////////////////////////////////////////////////////
function InstallSwig {

	if ! [ -x "${CACHED_DIR}/bin/swig" ]; then
      echo "Compiling swig"
      DownloadFile "https://ftp.osuosl.org/pub/blfs/conglomeration/swig/swig-3.0.12.tar.gz"  
      tar xzf swig-3.0.12.tar.gz 
      pushd swig-3.0.12 
      DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz"
      ./Tools/pcre-build.sh 
      ./configure --prefix=${CACHED_DIR}
      make -s -j 4 
      make install 
      popd
	fi
}

# //////////////////////////////////////////////////////
function InstallApache24 {

	APACHE_DIR=${CACHED_DIR}
	APR_DIR=${CACHED_DIR}

	if [ ! -f ${APACHE_DIR}/include/httpd.h ]; then
	
      echo "Compiling apache 2.4"	
	
		DownloadFile http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz
		tar xzf apr-1.6.5.tar.gz
		pushd apr-1.6.5
		./configure --prefix=${CACHED_DIR} && make -s && make install
		popd
		
		DownloadFile http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz
		tar xzf apr-util-1.6.1.tar.gz
		pushd apr-util-1.6.1
		./configure --prefix=${CACHED_DIR} --with-apr=${CACHED_DIR} && make -s && make install
		popd
		
		DownloadFile https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz
		tar xzf pcre-8.42.tar.gz
		pushd pcre-8.42
		./configure --prefix=${CACHED_DIR} && make -s && make install
		popd
		
		DownloadFile http://it.apache.contactlab.it/httpd/httpd-2.4.37.tar.gz
		tar xzf httpd-2.4.37.tar.gz
		pushd httpd-2.4.37
		./configure --prefix=${CACHED_DIR} --with-apr=${CACHED_DIR} --with-pcre=${CACHED_DIR} && make -s && make install
		popd
		
	fi
}

yum update 
yum install -y zlib-devel curl 

InstallOpenSSL 
InstallPython 
InstallCMake
InstallSwig

# for centos5 this is 2.2, I prefer to use 2.4 which is more common
# yum install -y httpd.x86_64 httpd-devel.x86_64
InstallApache24

PushCMakeOption OPENSSL_ROOT_DIR   ${OPENSSL_ROOT_DIR}
PushCMakeOption APACHE_DIR         ${APACHE_DIR}
PushCMakeOption APR_DIR            ${APR_DIR}
PushCMakeOptions

cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all 
cmake --build . --target test
cmake --build . --target install 

# deploy
# see CMake/deploy.py for details
cp ${OPENSSL_LIB_DIR}/libcrypto.so*      install/bin/
cp ${OPENSSL_LIB_DIR}/libssl.so*         install/bin/

# NOTE: sometimes docker containers do not contain the python shared library (needed for executables) so I'm copying it too

# NOTE2: if I use mine libpython* I will have problems with 'built in' modules (such as math) by running for example the executable "visus"
#         because for example my libpython* does nog have some builtin and OS one has it 
#         so it seems that mixing that is not a good idea
#         example: with pyenv math is not builtin and it's a shared library:
#         /root/.pyenv/versions/3.5.1/lib/python3.5/lib-dynload/math.cpython-35m-x86_64-linux-gnu.so
#         but in ubuntu I have math as builtin
#         if I use my manylinux libpython* and I use it in ubuntu, my manylinux try to find an external (not builtin) math,
#         but it does not exist!
#         in manylinux "python -c "import sys; print(sys.builtin_module_names)"
#             ('__builtin__', '__main__', '_codecs', '_sre', '_symtable', 'errno', 'exceptions', 'gc', 'imp', 'marshal', 'posix', 'pwd', 'signal', 'sys', 'thread', 'zipimport')
#         in ubuntu "import sys; print(sys.builtin_module_names)"
#             ('_ast', '_bisect', '_codecs', '_collections', ...., 'math', 'posix', ...., 'zipimport', 'zlib')
# cp $(pyenv prefix)/lib/libpython*        install/bin/  

cmake --build . --target deploy 

pushd install
LD_LIBRARY_PATH=$(pwd)/bin:$(dirname ${PYTHON_LIBRARY}) PYTHONPATH=$(pwd) bin/visus                                  && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd)/bin                              PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import OpenVisus" && echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}	
fi

if (( DEPLOY_PYPI == 1 )); then
	cmake --build ./ --target bdist_wheel --config ${CMAKE_BUILD_TYPE} 
	cmake --build ./ --target pypi        --config ${CMAKE_BUILD_TYPE}
fi








  
