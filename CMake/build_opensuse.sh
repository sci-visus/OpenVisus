#!/bin/bash

. "$(dirname "$0")/build_common.sh"

# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	zypper --non-interactive update
	zypper --non-interactive install sudo	
fi

sudo zypper --non-interactive update
sudo zypper --non-interactive install --type pattern devel_basis
sudo zypper --non-interactive install gcc-c++ cmake git swig  libuuid-devel libopenssl-devel curl patchelf
sudo zypper --non-interactive install apache2 apache2-devel

if (( VISUS_INTERNAL_DEFAULT == 0 )); then
	sudo zypper --non-interactive install zlib-devel liblz4-devel tinyxml-devel libfreeimage-devel libcurl-devel
fi

InstallPython 

if (( VISUS_GUI==1 )); then
	sudo zypper -n in  glu-devel  libQt5Concurrent-devel libQt5Network-devel libQt5Test-devel libQt5OpenGL-devel
fi

PushCmakeOptions
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 
cmake --build . --target deploy 

pushd install
LD_LIBRARY_PATH=$(pwd):$(dirname ${PYTHON_LIBRARY}) PYTHONPATH=$(pwd) bin/visus     && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd) PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import OpenVisus" && echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi


