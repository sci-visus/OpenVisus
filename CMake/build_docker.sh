#!/bin/bash

. "$(dirname "$0")/build_common.sh"

DOCKER_IMAGE=${DOCKER_IMAGE:-ubuntu:trusty} 

function PushDockerEnv {
	if [ -n "$2" ] ; then
		docker_opts+=" -e $1=$2"
	fi
}

# see build_common.sh to see what you want to 'forward'
docker_opts="" 
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

chmod +x CMake/build*.sh
sudo docker run --name mydocker -d -t -v $(pwd):/home/OpenVisus ${docker_opts} ${DOCKER_IMAGE} /bin/bash
sudo docker exec -ti   mydocker --workdir /home/OpenVisus                                      /bin/bash -c "./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ./ 
sudo chmod -R u+rwx           ./

