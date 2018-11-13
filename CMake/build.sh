#!/bin/bash

set -ex 

# detect platform
if [ -f /etc/os-release ]; then
	source /etc/os-release
	OS=$NAME
	OS_VERSION=$VERSION_ID

elif type lsb_release >/dev/null 2>&1; then
	OS=$(lsb_release -si)
	OS_VERSION=$(lsb_release -sr)

elif [ -f /etc/lsb-release ]; then
	source /etc/lsb-release
	OS=$DISTRIB_ID
	OS_VERSION=$DISTRIB_RELEASE

elif [ -f /etc/debian_version ]; then
	OS=Debian
	OS_VERSION=$(cat /etc/debian_version)
	
elif [ -f /etc/centos-release ]; then
	OS=$(cat /etc/centos-release)
	OS_VERSION=0 # todo

elif [ -f /etc/redhat-release ]; then
	OS=$(cat /etc/redhat-release)
	OS_VERSION=0 # todo
	
else
	OS=$(uname -s)
	OS_VERSION=$(uname -r)

fi

OS=$(echo ${OS} | tr '[:upper:]' '[:lower:]')

echo "OS         ${OS}"
echo "OS_VERSION ${OS_VERSION}"

source "$(dirname $0)/os/common.sh"

if [[ "${OS}" == *"darwin"* ]]; then
	OS_OSX=1
	source "$(dirname $0)/os/osx.sh"

elif [[ "${OS}" == *"ubuntu"* ]]; then
	OS_UBUNTU=1
	source "$(dirname $0)/os/ubuntu.sh"

elif [[ "${OS}" == *"opensuse"* ]]; then
	OS_OPENSUSE=1
	source "$(dirname $0)/os/opensuse.sh"
	
elif [[ "${OS}" == *"centos"* || "${OS}" == *"manylinux"* ]]; then
	OS_CENTOS=5
	source "$(dirname $0)/os/manylinux.sh"

else
	error "Not supported"
	
fi



