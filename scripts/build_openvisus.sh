#!/bin/bash

set -e  # stop or error
set -x  # very verbose

BUILD_DIR=${BUILD_DIR:-$(pwd)/build}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
Python_EXECUTABLE=${Python_EXECUTABLE:-python} 

GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)
OPENVISUS_TAG=`${Python_EXECUTABLE} Libs/swig/setup.py print-tag`
PYTHON_VERSION=${"import sys;print(f'{sys.version_info.major}{sys.version_info.minor}')"}

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR} 

cmake \
	-DPython_EXECUTABLE=${Python_EXECUTABLE} \
	-DQt5_DIR=${Qt5_DIR} \
	-DVISUS_GUI=${VISUS_GUI} \
	-DVISUS_MODVISUS=${VISUS_MODVISUS} \
	-DVISUS_SLAM=${VISUS_SLAM} \
	../

cmake --build ./ --target all     --config Release --parallel 4

cmake --build ./ --target install --config Release

export PYTHONPATH=./Release

${Python_EXECUTABLE} -m OpenVisus configure || true # segmentation fault problem
${Python_EXECUTABLE}  -m OpenVisus test

# this is just to check the linking, not producing anything
if [[ "${VISUS_GUI}" == "1" ]] ; then 
	${Python_EXECUTABLE}  -m OpenVisus test-gui 
fi

# create the wheel and upload
if [[ "${GIT_TAG}" != "" && "${PYPI_USERNAME}"!="" && "${PYPI_PASSWORD}"!="" && "${PLATFORM_NAME}"!="" ]] ; then
	pushd build/Release/OpenVisus
	${Python_EXECUTABLE} -m pip install -q setuptools wheel twine
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION} --plat-name==${PLATFORM_NAME}
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing  "dist/*.whl" 
	popd
fi
 
# for python version see Docker file (it's 3.7 on http:2.4 / debian:buster-slim)
if [[ "${GIT_TAG}" != "" && "${PYTHON_VERSION}" == "37" && "${DOCKER_TOKEN}" != "" && "${DOCKER_USERNAME}" != "" ]] ; then
	pushd Docker/mod_visus
	sleep 30 # give time pypi to get the pushed file
	docker build --tag visus/mod_visus:$OPENVISUS_TAG --tag visus/mod_visus:latest --build-arg OPENVISUS_TAG=${OPENVISUS_TAG} .
	echo ${DOCKER_USERNAME} | docker login -u=${DOCKER_TOKEN} --password-stdin
	docker push visus/mod_visus:$OPENVISUS_TAG
	docker push visus/mod_visus:latest
 popd
fi

if [[ "${GIT_TAG}" != "" && "${ANACONDA_TOKEN}" != ""  ]] ; then
	pushd build/Release/OpenVisus
	conda install --yes anaconda-client conda-build wheel 1>/dev/null || true
	rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2")  || true
	python setup.py -q bdist_conda 1>/dev/null
	CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"  | head -n 1
	export PATH=$HOME/anaconda3/bin:$PATH
	anaconda --verbose --show-traceback  -t ${ANACONDA_TOKEN} upload "${CONDA_FILENAME}"
	popd
fi

