#!/bin/bash
set -e  # stop or error
set -x  # very verbose
mkdir -p build
cd build 
cmake -DPython_EXECUTABLE=$(which python${PYTHON_VERSION}) -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4
cmake --build ./ --target install --config Release

