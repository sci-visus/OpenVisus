#!/bin/bash

source "$(dirname "$0")/build_common.sh"

if [[ $DOCKER_IMAGE == "" ]] ; then
	echo "Please specify a docker image (i.e. set DOCKER_IMAGE)"
	exit -1
fi

# see build_common.sh to see what you want to 'forward'
SOURCE_DIR=$(pwd)

# see build_common.sh for stuff you want to forward
docker_env=""

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

sudo docker run -d -ti --name mydocker -v ${SOURCE_DIR}:${SOURCE_DIR} ${docker_env} ${DOCKER_IMAGE} /bin/bash
sudo docker exec mydocker /bin/bash -c "cd ${SOURCE_DIR} && ./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ${BUILD_DIR}
sudo chmod -R u+rwx           ${BUILD_DIR}

