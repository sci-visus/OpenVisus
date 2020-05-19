#!/bin/bash

set -e  # stop or error
set -x  # very verbose

PYTHON_VERSION=${PYTHON_VERSION:-3.6}
Python_EXECUTABLE=$(which python${PYTHON_VERSION})
Qt5_DIR=${Qt5_DIR:-/opt/qt59}
VISUS_SLAM=${VISUS_SLAM:-0}

${Python_EXECUTABLE} -m pip install numpy setuptools wheel twine --upgrade 1>/dev/null || true

mkdir -p build 
cd build

cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} -DVISUS_SLAM=${VISUS_SLAM} ../
cmake --build ./ --target all     --config Release --parallel 4

cd Release/OpenVisus
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus configure || true # segmentation fault problem
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus convert

GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)
if [[ "${GIT_TAG}" != "" && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != "" ]] ; then
	 ${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=manylinux2010_x86_64
	 ${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi


