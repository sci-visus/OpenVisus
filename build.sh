#!/bin/bash

this_dir=$(dirname $0)

if [ $(uname) = "Darwin" ]; then
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

	

