#!/bin/bash

# stop on error
set -e

# /////////////////////////////////////////////////////
# Build the OpenVisus docker images.
# Example:
#
#   cd Docker
#   sudo ./build_all.sh
# /////////////////////////////////////////////////////

function BuildDocker {
	name=$1
	echo "# ///////////////////////////////////////////////////////////"
	echo "# Building ${name}.."
	docker build --tag ${name} --file Dockerfile.${name} .
	echo "# Done ${name}"
}

# These three images use pip to install OpenVisus:
BuildDocker mod_visus-ubuntu   
BuildDocker mod_visus-opensuse 

# this does not work, now we have a conda distribution without mod_visus
# do we need to support Apache/mod_visus for conda too?
# BuildDocker anaconda           

# These three build from scratch (without pip binary distribution)
BuildDocker ubuntu
BuildDocker opensuse
BuildDocker manylinux


