#!/bin/bash

. "$(dirname "$0")/build_common.sh"

if [[ $DOCKER_IMAGE == "" ]] ; then
	echo "Please specify a docker image (i.e. set DOCKER_IMAGE)"
	exit -1
fi

# see build_common.sh to see what you want to 'forward'
PushDockerEnvs

docker_opts+=" -ti"
docker_opts+=" -v $(pwd):/home/OpenVisus"

sudo docker run   -d --name mydocker ${docker_opts} ${DOCKER_IMAGE} /bin/bash
sudo docker exec            mydocker ${docker_opts}                 /bin/bash -c "cd /home/OpenVisus && chmod +x ./CMake/build*.sh && ./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ${BUILD_DIR}
sudo chmod -R u+rwx           ${BUILD_DIR}

