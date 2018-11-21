#!/bin/bash

. "$(dirname "$0")/build_common.sh"

DOCKER_IMAGE=${DOCKER_IMAGE:-ubuntu:trusty} 
CONTAINER_NAME=${CONTAINER_NAME:-mydocker}

docker_opts="" 
PushDockerEnvs
chmod +x CMake/build*.sh
sudo docker run --name ${CONTAINER_NAME} -d -t -v $(pwd):/home/OpenVisus ${docker_opts} ${DOCKER_IMAGE} /bin/bash
sudo docker exec -ti   ${CONTAINER_NAME} --workdir /home/OpenVisus                                      /bin/bash -c "./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ./ 
sudo chmod -R u+rwx           ./

