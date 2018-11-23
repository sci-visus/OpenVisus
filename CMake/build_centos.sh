#!/bin/bash

source "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# note: only centos5 is supported right now

yum update 
yum install -y zlib-devel curl 

# for centos5 this is 2.2
# yum install -y httpd.x86_64 httpd-devel.x86_64
InstallApache24
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








  
