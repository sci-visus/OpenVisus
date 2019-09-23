#!/bin/bash

# to debug:
# sudo docker run -ti -v $(pwd):/root/OpenVisus quay.io/pypa/manylinux1_x86_64 /bin/bash
# cd /root/OpenVisus
# BUILD_DIR=$(pwd)/build_docker ./CMake/build.sh


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

DOCKER_IMAGE=${DOCKER_IMAGE:-quay.io/pypa/manylinux1_x86_64}

BUILD_DIR=$(pwd)/build_docker
mkdir -p ${BUILD_DIR}

# note: sudo is needed anyway otherwise travis fails
sudo docker rm -f mydocker 2>/dev/null || true

declare -a docker_opts

# share souce directory and build directory
docker_opts+=(-v $(pwd):/root/OpenVisus)
docker_opts+=(-e BUILD_DIR=/root/OpenVisus/build_docker)

if [[ "${CMAKE_BUILD_TYPE}" != "" ]] ; then
	docker_opts+=(-e CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})
fi

if [[ "${VISUS_NET}" != "" ]] ; then
	docker_opts+=(-e VISUS_NET=${VISUS_NET})
fi

if [[ "${VISUS_IMAGE}" != "" ]] ; then
	docker_opts+=(-e VISUS_IMAGE=${VISUS_IMAGE})
fi

if [[ "${VISUS_PYTHON}" != "" ]] ; then
	docker_opts+=(-e VISUS_PYTHON=${VISUS_PYTHON})

	if [[ "${PYTHON_VERSION}" != "" ]] ; then
		docker_opts+=(-e PYTHON_VERSION=${PYTHON_VERSION})
	fi
fi

if [[ "${VISUS_GUI}" != "" ]] ; then
	docker_opts+=(-e VISUS_GUI=${VISUS_GUI})
fi

# this is needed to forward the deploy to the docker image (where I have a python to do the deploy)
# deploy to pypi | github
if (( DEPLOY_PYPI == 1 )) ; then
	docker_opts+=(-e DEPLOY_PYPI=${DEPLOY_PYPI})
	docker_opts+=(-e PYPI_USERNAME=${PYPI_USERNAME})
	docker_opts+=(-e PYPI_PASSWORD=${PYPI_PASSWORD})
fi

if (( DEPLOY_GITHUB == 1 )) ; then
	docker_opts+=(-e DEPLOY_GITHUB=${DEPLOY_GITHUB})
	docker_opts+=(-e GITHUB_API_TOKEN=${GITHUB_API_TOKEN})
	docker_opts+=(-e TRAVIS_TAG=${TRAVIS_TAG})
fi

sudo docker run -d -ti --name mydocker ${docker_opts[@]} ${DOCKER_IMAGE} /bin/bash

sudo docker exec mydocker /bin/bash -c "cd /root/OpenVisus && ./CMake/build.sh"

# fix permissions
sudo chown -R "$USER":"$USER" ${BUILD_DIR} 1>/dev/null && :
sudo chmod -R u+rwx           ${BUILD_DIR} 1>/dev/null && :

echo "OpenVisus CMake/build_docker.sh finished"





