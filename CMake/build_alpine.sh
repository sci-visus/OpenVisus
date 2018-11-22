#!/bin/sh

# override
export VISUS_GUI=0
export DISABLE_OPENMP=1
export PYTHON_VERSION=3.6

. "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# to try it step by step
# docker run -it --entrypoint=/bin/sh alpine:3.7

# ////////////////////////////////////////////////
# dependencies
# ////////////////////////////////////////////////

apk add --no-cache python3 apache2 curl libstdc++
apk add --no-cache --virtual OpenVisusBuildDeps alpine-sdk swig apache2-dev curl-dev python3-dev git cmake
pip3 install --upgrade pip 
pip3 install numpy 

cd $BUILD_DIR && mkdir -p ./build && cd ./build 

PushCMakeOptions
PushCMakeOption VISUS_INTERNAL_ZLIB      1
PushCMakeOption VISUS_INTERNAL_LZ4       1
PushCMakeOption VISUS_INTERNAL_TINYXML   1
PushCMakeOption VISUS_INTERNAL_FREEIMAGE 1
PushCMakeOption VISUS_INTERNAL_OPENSSL   0
PushCMakeOption VISUS_INTERNAL_CURL      0
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4 
cmake --build . --target install 

# no deploy here 
# cmake --build . --target deploy 

pushd install
LD_LIBRARY_PATH=$(pwd)/bin:$(dirname ${PYTHON_LIBRARY}) PYTHONPATH=$(pwd) bin/visus                                  && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd)/bin                              PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import OpenVisus" && echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi