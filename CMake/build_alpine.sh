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

# no deploy here 
# cmake --build . --target deploy 

cd install
LD_LIBRARY_PATH=$(pwd)/bin PYTHONPATH=$(pwd) bin/visus                                  && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd)/bin PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import VisusKernelPy" && echo "Extending working"
cd ..

if [ "$DEPLOY_GITHUB" = "1" ]; then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi
