#!/bin/bash

set -ex 

if [ "${DOCKER_SHELL}" == "" ] ; then DOCKER_SHELL=/bin/bash ; fi
if [ "${BUILD_DIR}"    == "" ] ; then BUILD_DIR=$(pwd)/build ; fi

# pass thought environment variables
DOCKER_ENV=""
if [ "${PYTHON_VERSION}"         != "" ] ; then DOCKER_ENV="${DOCKER_ENV} -e PYTHON_VERSION=${PYTHON_VERSION}" ; fi
if [ "${VISUS_INTERNAL_DEFAULT}" != "" ] ; then DOCKER_ENV="${DOCKER_ENV} -e VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT}" ; fi
if [ "${BUILD_DIR}"              != "" ] ; then DOCKER_ENV="${DOCKER_ENV} -e BUILD_DIR=${BUILD_DIR}" ; fi

this_dir=$(pwd)
sudo docker rm -f mydocker 2>/dev/null || true
sudo docker run -d -ti --name mydocker -v ${this_dir}:${this_dir} ${DOCKER_ENV} ${DOCKER_IMAGE} ${DOCKER_SHELL}
sudo docker exec mydocker ${DOCKER_SHELL} -c "cd ${this_dir} && ./build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ${BUILD_DIR}
sudo chmod -R u+rwx           ${BUILD_DIR}

