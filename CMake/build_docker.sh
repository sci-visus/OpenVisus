#!/bin/bash

# see build_common.sh for stuff you want to forward
# NOTE: if you specify BUILD_DIR it must be inside $(pwd) otherwise when the container exit you will loose the build

DOCKER_IMAGE=${DOCKER_IMAGE:-ubuntu:trusty}
DOCKER_SHELL=${DOCKER_SHELL:-/bin/bash}

source "$(dirname "$0")/build_common.sh"

docker_env=""

# //////////////////////////////////////////////////////
function PushDockerEnv {
	if [ -n "$2" ] ; then
		docker_env+=" -e $1=$2"
	fi
}

PushDockerEnv PYTHON_VERSION          ${PYTHON_VERSION}
PushDockerEnv VISUS_INTERNAL_DEFAULT  ${VISUS_INTERNAL_DEFAULT}
PushDockerEnv DISABLE_OPENMP          ${DISABLE_OPENMP}
PushDockerEnv VISUS_GUI               ${VISUS_GUI}
PushDockerEnv CMAKE_BUILD_TYPE        ${CMAKE_BUILD_TYPE}

PushDockerEnv DEPLOY_GITHUB           ${DEPLOY_GITHUB}

PushDockerEnv DEPLOY_PYPI             ${DEPLOY_PYPI}
PushDockerEnv PYPI_USERNAME           ${PYPI_USERNAME}
PushDockerEnv PYPI_PASSWORD           ${PYPI_PASSWORD}
PushDockerEnv PYPI_PLAT_NAME          ${PYPI_PLAT_NAME}

PushDockerEnv BUILD_DIR               ${BUILD_DIR}

SOURCE_DIR=$(pwd)
sudo docker run -d -ti --name mydocker -v ${SOURCE_DIR}:${SOURCE_DIR} ${docker_env} ${DOCKER_IMAGE} ${DOCKER_SHELL}
sudo docker exec mydocker ${DOCKER_SHELL} -c "cd ${SOURCE_DIR} && ./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ${BUILD_DIR}
sudo chmod -R u+rwx           ${BUILD_DIR}

