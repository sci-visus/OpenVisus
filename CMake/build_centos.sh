#!/bin/bash

. "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# note: only centos5 is supported right now

yum update 
yum install -y zlib-devel curl 
yum install -y httpd.x86_64 httpd-devel.x86_64

InstallOpenSSL 
InstallPython 
InstallCMake
InstallSwig

# broken right now
if (( VISUS_GUI == 1 )); then
	error "TODO"  
fi

cmake_opts=""
PushCmakeOptions
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 

# deploy
if (( 0 )); then
	# WRONG copying very low-level libraries is wrong (i.e. crashes)! I need only distribute real dependencies
	# cmake --build . --target deploy 
	echo "nop"
else
	# NOTE: sometimes docker containers do not contain the python shared library (needed for executables) so I'm copying it too
	cp ${OPENSSL_LIB_DIR}/libcrypto.so*      install/bin/
	cp ${OPENSSL_LIB_DIR}/libssl.so*         install/bin/
	cp $(pyenv prefix)/lib/libpython* install/bin/ 
fi

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








  
