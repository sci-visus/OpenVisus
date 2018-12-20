#!/bin/sh
set -ex 

this_dir=$(dirname $0)
s
if [ -n "${DOCKER_IMAGE}" ]; then

	DOCKER_SHELL=${DOCKER_SHELL:-/bin/bash}
	DOCKER_ENV=${DOCKER_ENV:-}
	BUILD_DIR=${BUILD_DIR:-build}

	if [ "${PYTHON_VERSION}"         != "" ] ; then DOCKER_ENV="${DOCKER_ENV} -e PYTHON_VERSION=${PYTHON_VERSION}" ; fi
	if [ "${VISUS_INTERNAL_DEFAULT}" != "" ] ; then DOCKER_ENV="${DOCKER_ENV} -e VISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT}" ; fi
	if [ "${BUILD_DIR}"              != "" ] ; then DOCKER_ENV="${DOCKER_ENV} -e BUILD_DIR=${BUILD_DIR}" ; fi

	sudo docker rm -f mydocker || true
	sudo docker run -d -ti --name mydocker -v $(pwd):/home/OpenVisus ${DOCKER_ENV} ${DOCKER_IMAGE} ${DOCKER_SHELL}
	sudo docker exec mydocker ${DOCKER_SHELL} -c "cd /home/OpenVisus && ./build.sh"

	sudo chown -R "$USER":"$USER" ${BUILD_DIR}
	sudo chmod -R u+rwx           ${BUILD_DIR}

elif [ $(uname) = "Darwin" ]; then
	${this_dir}/CMake/build_osx.sh

elif [ -x "$(command -v apt-get)" ]; then
	${this_dir}/CMake/build_ubuntu.sh

elif [ -x "$(command -v zypper)" ]; then
	${this_dir}/CMake/build_opensuse.sh
	
elif [ -x "$(command -v yum)" ]; then
	${this_dir}/CMake/build_manylinux.sh

elif [ -x "$(command -v apk)" ]; then
	${this_dir}/CMake/build_alpine.sh

else
	echo "Failed to detect OS version"
fi

	

