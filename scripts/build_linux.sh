#!/bin/bash

set -e  # stop or error
set -x  # very verbose

PYTHON_VERSION=${PYTHON_VERSION:-3.6}
Python_EXECUTABLE=${Python_EXECUTABLE:-python${PYTHON_VERSION}}  
Qt5_DIR=${Qt5_DIR:-/opt/qt59}

${Python_EXECUTABLE} -m pip install numpy setuptools wheel twine --upgrade 1>/dev/null || true

mkdir -p build 
cd build

cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4
cmake --build ./ --target install --config Release

cd Release/OpenVisus

PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus convert

GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)
if [[ "${GIT_TAG}" != "" && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != "" ]] ; then
   PLATFORM_NAME=manylinux2010_x86_64
   PLATFORM_TAG=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1}     
	 ${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=${PLATFORM_TAG} --plat-name=${PLATFORM_NAME}
	 ${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi




