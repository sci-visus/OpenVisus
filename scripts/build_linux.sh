#!/bin/bash

set -e
set -x

# settings
BUILD_DIR=${BUILD_DIR:-$(pwd)/build}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_NET=${VISUS_NET:-1}
VISUS_IMAGE=${VISUS_IMAGE:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
Python_EXECUTABLE=${Python_EXECUTABLE:-python3} 
GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)
OPENVISUS_TAG=`${Python_EXECUTABLE} Libs/swig/setup.py print-tag`
PYTHON_VERSION=`${Python_EXECUTABLE} -c "import sys;print(f'{sys.version_info.major}{sys.version_info.minor}')"`

# compile
mkdir -p ${BUILD_DIR}
pushd ${BUILD_DIR} 
cmake \
	-DPython_EXECUTABLE=${Python_EXECUTABLE} \
	-DQt5_DIR=${Qt5_DIR} \
	-DVISUS_GUI=${VISUS_GUI} \
	-DVISUS_MODVISUS=${VISUS_MODVISUS} \
	-DVISUS_SLAM=${VISUS_SLAM} \
	-DVISUS_NET=${VISUS_NET} \
	-DVISUS_IMAGE=${VISUS_IMAGE} \
	../
cmake --build ./ --target all     --config Release --parallel 4
cmake --build ./ --target install --config Release
popd

# test
export PYTHONPATH=${BUILD_DIR}/Release
${Python_EXECUTABLE} -m OpenVisus configure || true # segmentation fault problem
${Python_EXECUTABLE} -m OpenVisus test
if [[ "${VISUS_GUI}" == "1" ]] ; then 
	${Python_EXECUTABLE}  -m OpenVisus test-gui  # this is just to check the linking, not producing anything
fi
unset PYTHONPATH

# PyPi
if [[ "${GIT_TAG}"!="" && "${PYPI_USERNAME}"!="" && "${PYPI_PASSWORD}"!=""]] ; then
	pushd ${BUILD_DIR}/Release/OpenVisus
	${Python_EXECUTABLE} -m pip install -q setuptools wheel twine
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION} --plat-name==${PYPI_PLATFORM_NAME}
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing  "dist/*.whl" 
	popd
	
	# Docker visus/mod_visus
	if [[ "${DOCKER_USERNAME}"!="" && "${DOCKER_TOKEN}"!="" ]] ; then 
		sleep 30 # give time pypi to get the pushed file
		pushd Docker/mod_visus
		docker build --tag visus/mod_visus:$OPENVISUS_TAG --tag visus/mod_visus:latest --build-arg OPENVISUS_TAG=${OPENVISUS_TAG} .
		echo ${DOCKER_USERNAME} | docker login -u=${DOCKER_TOKEN} --password-stdin
		docker push visus/mod_visus:$OPENVISUS_TAG
		docker push visus/mod_visus:latest	
		popd
	fi
fi

# conda
if [[ "${GIT_TAG}"!="" && "${ANACONDA_TOKEN}" != ""  ]] ; then
	pushd ${BUILD_DIR}/Release/OpenVisus
	conda install --yes anaconda-client conda-build wheel 1>/dev/null || true
	rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2")  || true
	python setup.py -q bdist_conda 1>/dev/null
	CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"  | head -n 1
	export PATH=$HOME/anaconda3/bin:$PATH
	anaconda --verbose --show-traceback  -t ${ANACONDA_TOKEN} upload "${CONDA_FILENAME}"
	popd
fi

