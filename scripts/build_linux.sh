#!/bin/bash
set -e  # stop or error
set -x  # very verbose

BUILD_DIR=${BUILD_DIR:-$(pwd)/build}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:1}
VISUS_SLAM=${VISUS_SLAM:-1}

Python_EXECUTABLE=$(which python${PYTHON_VERSION})
mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR} 
cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} -DVISUS_GUI=${VISUS_GUI} -DVISUS_MODVISUS=${VISUS_MODVISUS} -DVISUS_SLAM=${VISUS_SLAM} ../
cmake --build ./ --target all     --config Release --parallel 4
cmake --build ./ --target install --config Release

cd Release/OpenVisus
export PYTHONPATH=../
${Python_EXECUTABLE} -m OpenVisus configure || true  # segmentation fault problem
