#!/bin/bash

set -ex 

if [ -n "${DOCKER_IMAGE}" ]; then
	./CMake/build_docker.sh

elif [ $(uname) = "Darwin" ]; then
	./CMake/build_osx.sh

elif [ -x "$(command -v apt-get)" ]; then
	./CMake/build_ubuntu.sh

elif [ -x "$(command -v zypper)" ]; then
	./CMake/build_opensuse.sh
	
elif [ -x "$(command -v yum)" ]; then
	./CMake/build_manylinux.sh

else
	echo "Failed to detect OS version"
fi

