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

# run docker
docker run --rm -v ${PWD}:/home/OpenVisus -w /home/OpenVisus \
  -e BUILD_DIR=build_docker \
  -e PYTHON_VERSION=${PYTHON_VERSION} \
  -e PYPI_USERNAME=${PYPI_USERNAME} -e PYPI_PASSWORD=${PYPI_PASSWORD} \
  -e ANACONDA_TOKEN=${ANACONDA_TOKEN} \
  -e VISUS_GUI=${VISUS_GUI} -e VISUS_SLAM=${VISUS_SLAM} -e VISUS_MODVISUS=${VISUS_MODVISUS} \
  -e INSIDE_DOCKER=1 \
  ${PULL_IMAGE} bash scripts/build_ubuntu.docker.sh

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
  

