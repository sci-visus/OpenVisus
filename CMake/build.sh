#!/bin/bash

THIS_DIR=$(dirname $0)

if [[ $(uname) == "Darwin" ]]; then
	$THIS_DIR/build_osx.sh

elif [ -n "$(which zypper)" ]; then
	$THIS_DIR/build_opensuse.sh
	
elif [ -n "$(which apt-get)" ]; then
	$THIS_DIR/build_ubuntu.sh

elif [ -n "$(which yum)" ]; then
	$THIS_DIR/build_centos.sh

else
	echo "Failed to detect OS version"
fi

	

