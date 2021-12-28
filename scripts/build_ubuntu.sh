#!/bin/bash

set -e
set -x

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# /////////////////////////////////////////////
# arguments

BUILD_DIR=${BUILD_DIR:-build_docker}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
PYTHON_VERSION=${PYTHON_VERSION:-3.8}
Qt5_DIR=${Qt5_DIR:-/opt/qt512}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_PASSWORD=${PYPI_PASSWORD:-}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}
DOCKER_TOKEN=${DOCKER_TOKEN:-}
DOCKER_USERNAME=${DOCKER_USERNAME:-}
PULL_IMAGE=${PULL_IMAGE:-visus/portable-linux-binaries_x86_64:4.1}
PUSH_IMAGE=${PUSH_IMAGE:-visus/mod_visus}

if [[ "${INSIDE_DOCKER}" == "" ]] ; then
 
  # run docker
  docker run --rm -v ${PWD}:/home/OpenVisus -w /home/OpenVisus \
    -e BUILD_DIR=build_docker \
    -e PYTHON_VERSION=${PYTHON_VERSION} \
    -e PYPI_USERNAME=${PYPI_USERNAME} -e PYPI_PASSWORD=${PYPI_PASSWORD} \
    -e ANACONDA_TOKEN=${ANACONDA_TOKEN} \
    -e VISUS_GUI=${VISUS_GUI} -e VISUS_SLAM=${VISUS_SLAM} -e VISUS_MODVISUS=${VISUS_MODVISUS} \
    -e INSIDE_DOCKER=1 \
    ${PULL_IMAGE} bash scripts/build_ubuntu.sh

  # modvisus
  if [[ "${GIT_TAG}" != "" &&  "${PYTHON_VERSION}" == "3.9" ]] ; then
    sleep 30 # give time for pip package to be ready
    pushd  Docker/mod_visus
    docker build --tag PUSH_IMAGE:${GIT_TAG} --tag PUSH_IMAGE:latest --build-arg TAG=${GIT_TAG} .
    echo ${DOCKER_TOKEN} | docker login -u=${DOCKER_USERNAME} --password-stdin
    docker push PUSH_IMAGE:${GIT_TAG}
    docker push PUSH_IMAGE:latest
    popd
  fi
  
else
  
  # compile inside docker
  
  source $(dirname "$0")/utils.sh

  ARCHITECTURE=`uname -m`
  PIP_PLATFORM=unknown
  if [[ "${ARCHITECTURE}" ==  "x86_64" ]] ; then PIP_PLATFORM=manylinux2010_${ARCHITECTURE} ; fi
  if [[ "${ARCHITECTURE}" == "aarch64" ]] ; then PIP_PLATFORM=manylinux2014_${ARCHITECTURE} ; fi

  # *** cpython ***
  PYTHON=`which python${PYTHON_VERSION}`

  mkdir -p ${BUILD_DIR} 
  cd ${BUILD_DIR}
  cmake -DPython_EXECUTABLE=${PYTHON} -DQt5_DIR=${Qt5_DIR} -DVISUS_GUI=${VISUS_GUI} -DVISUS_MODVISUS=${VISUS_MODVISUS} -DVISUS_SLAM=${VISUS_SLAM} ../
  make -j 
  make install

  pushd Release/OpenVisus
  ConfigureAndTestCPython 
  DistribToPip
  popd

  if [[ "${VISUS_GUI}" == "1" ]]; then
  	CreateNonGuiVersion
  	pushd Release.nogui/OpenVisus 
  	ConfigureAndTestCPython 
  	DistribToPip 
  	popd
  fi

  # *** conda ***
  InstallCondaUbuntu || true # can exist
  ActivateConda
  PYTHON=`which python`

  # # fix for bdist_conda problem
  cp -n \
  	${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/distutils/command/bdist_conda.py \
  	${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/site-packages/setuptools/_distutils/command/bdist_conda.py

  pushd Release/OpenVisus 
  ConfigureAndTestConda 
  DistribToConda 
  popd

  if [[ "${VISUS_GUI}" == "1" ]]; then
  	pushd Release.nogui/OpenVisus
  	ConfigureAndTestConda
  	DistribToConda
  	popd
  fi

fi


