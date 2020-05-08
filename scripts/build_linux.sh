#!/bin/bash

# to test:
# sudo docker run --rm -it -v $(pwd):/root/OpenVisus -w /root/OpenVisus visus/travis-image /bin/bash 
# export PYTHON_VERSION=3.8
# ./scripts/build_linux.sh
# cd build_linux/Release/OpenVisus
# scripts/build_conda.sh

set -e  # stop or error
set -x  # very verbose

# see my manylinux image
export Python_EXECUTABLE=python${PYTHON_VERSION}  
export Qt5_DIR=/opt/qt59
export GIT_TAG=${GIT_TAG:-}

mkdir -p build_linux && cd build_linux
cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release --parallel 4
cmake --build ./ --target install --config Release

cd Release/OpenVisus

PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus convert

# wheel
if [[ "${GIT_TAG}" != "" && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != "" ]] ; then
	${Python_EXECUTABLE} -m pip install setuptools wheel --upgrade || true
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=manylinux2010_x86_64
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi




