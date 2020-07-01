#!/bin/bash
set -e  # stop or error
set -x  # very verbose

Python_EXECUTABLE=$(which python${PYTHON_VERSION})
mkdir -p build
cd build 
cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4
cmake --build ./ --target install --config Release

cd Release/OpenVisus
export PYTHONPATH=../
${Python_EXECUTABLE} -m OpenVisus configure || true  # segmentation fault problem
