#!/bin/bash

# see build_common.sh for stuff you want to forward
# NOTE: if you specify BUILD_DIR it must be inside $(pwd) otherwise when the container exit you will loose the build

DOCKER_IMAGE=${DOCKER_IMAGE:-ubuntu:trusty}
DOCKER_SHELL=${DOCKER_SHELL:-/bin/bash}

source "$(dirname "$0")/build_common.sh"

docker_env=""

PushDockerEnvs

SOURCE_DIR=$(pwd)
sudo docker run -d -ti --name mydocker -v ${SOURCE_DIR}:${SOURCE_DIR} ${docker_env} ${DOCKER_IMAGE} ${DOCKER_SHELL}
sudo docker exec mydocker ${DOCKER_SHELL} -c "cd ${SOURCE_DIR} && ./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ${BUILD_DIR}
sudo chmod -R u+rwx           ${BUILD_DIR}

