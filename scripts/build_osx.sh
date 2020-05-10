#!/bin/bash

set -e  # stop or error
set -x  # very verbose

mkdir -p build && cd build
cmake -GXcode -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} ../
cmake --build ./ --target ALL_BUILD --config Release --parallel 4 
cmake --build ./ --target install --config Release

cd Release/OpenVisus

PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus convert

GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)
if [[ "${GIT_TAG}" != "" && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != ""  ]] ; then
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=macosx_10_9_x86_64
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi






