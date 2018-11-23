#!/bin/bash

source "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# note: only centos5 is supported right now

yum update 
yum install -y zlib-devel curl 

if [[ ${APACHE_VERSION} == "" ]]; then
	APACHE_VERSION=2.2
fi

if [[ ${APACHE_VERSION} == "2.2" ]]; then

	# for centos5 this is 2.2
	yum install -y httpd.x86_64 httpd-devel.x86_64

elif [[ ${APACHE_VERSION} == "2.4" ]]; then

	downloadFile http://mirror.nohup.it/apache/apr/apr-1.6.5.tar.gz
	tar -xvzf apr-1.6.5.tar.gz
	pushd apr-1.6.5
	./configure && make && make install
	popd
	
	downloadFile http://mirror.nohup.it/apache/apr/apr-util-1.6.1.tar.gz
	tar -xvzf apr-util-1.6.1.tar.gz
	pushd apr-util-1.6.1
	./configure --with-apr=/usr/local/apr && make && make install
	popd
	
	downloadFile https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz
	tar -xvzf pcre-8.42.tar.gz
	pushd pcre-8.42
	./configure --prefix=/usr/local/pcre && make && make install
	popd
	
	downloadFile http://it.apache.contactlab.it/httpd/httpd-2.4.37.tar.gz
	tar -xvzf httpd-2.4.37.tar.gz
	pushd httpd-2.4.37
	./configure --with-apr=/usr/local/apr/ --with-pcre=/usr/local/pcre && make&& make install
	popd

else
	echo "Unsupported APACHE_VERSION ${APACHE_VERSION}"
	exit 0
endif

InstallOpenSSL 
InstallPython 
InstallCMake
InstallSwig

# broken right now
if (( VISUS_GUI == 1 )); then
	error "TODO"  
fi

PushCMakeOptions
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 

# deploy
# NOTE: sometimes docker containers do not contain the python shared library (needed for executables) so I'm copying it too
# see CMake/deploy.py for details
cp ${OPENSSL_LIB_DIR}/libcrypto.so*      install/bin/
cp ${OPENSSL_LIB_DIR}/libssl.so*         install/bin/
cp $(pyenv prefix)/lib/libpython*        install/bin/  
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








  
