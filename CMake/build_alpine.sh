#!/bin/sh

# override
VISUS_INTERNAL_DEFAULT=1
DISABLE_OPENMP=1
VISUS_GUI=0

source "$(dirname "$0")/build_common.sh"

SOURCE_DIR=$(pwd)
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# to try it step by step
# docker run -it alpine:3.7 /bin/sh

# ////////////////////////////////////////////////
# dependencies
# ////////////////////////////////////////////////

apk add --no-cache python3 apache2 curl libstdc++
apk add --no-cache --virtual OpenVisusBuildDeps alpine-sdk swig apache2-dev curl-dev python3-dev git cmake
pip3 install --upgrade pip 
pip3 install numpy 

PushCMakeOptions
PushCMakeOption VISUS_INTERNAL_OPENSSL 0
PushCMakeOption VISUS_INTERNAL_CURL    0
cmake ${cmake_opts} ${SOURCE_DIR} 

cmake --build . --target all -- -j 4 
cmake --build . --target test
cmake --build . --target install

cd install
${PYTHON_EXECUTABLE} BundleUtils.py --pip-post-install
./visus.sh                                 && echo "Embedding working"
${PYTHON_EXECUTABLE} -c "import OpenVisus" && echo "Extending working"
cd ..


