#!/bin/bash

__to_test__="
docker run --rm -it \
   -e BUILD_DIR=build_docker \
   -e PYTHON_VERSION=3.8 \
   -e VISUS_GUI=1 -e VISUS_SLAM=1 -e VISUS_MODVISUS=1
   -e PYPI_USERNAME=scrgiorgio -e PYPI_PASSWORD=XXXX \
   -e ANACONDA_TOKEN=ZZZZ \
   -v $PWD:/home/OpenVisus -w /home/OpenVisus \
   visus/portable-linux-binaries_x86_64:4.1 bash

./scripts/build_linux.sh
"

set -e  # stop or error
set -x  # very verbose

source $(dirname "$0")/utils.sh

BUILD_DIR=${BUILD_DIR:-build_docker}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
Qt5_DIR=${Qt5_DIR:-/opt/qt512}

GIT_TAG=${GIT_TAG:-`git describe --tags --exact-match 2>/dev/null || true`}

# *** cpython ***
PYTHON=`which python${PYTHON_VERSION}`
ARCHITECTURE=`uname -m`
if [[ "${ARCHITECTURE}" == "x86_64" ]] ; then
	PIP_PLATFORM=manylinux2010_${ARCHITECTURE}
elif [[ "${ARCHITECTURE}" == "aarch64" ]] ; then
	PIP_PLATFORM=manylinux2014_${ARCHITECTURE}
else
	echo "unknown architecture [${ARCHITECTURE}]"
	exit -1
fi	

BuildOpenVisusUbuntu

pushd Release/OpenVisus
ConfigureAndTestCPython 
DistribToPip
popd

if [[ "$VISUS_GUI" == "1" ]]; then
	CreateNonGuiVersion

	pushd Release.nogui/OpenVisus 
	ConfigureAndTestCPython 
	DistribToPip 
	popd
fi

# *** conda ***
InstallCondaUbuntu
ActivateConda
PYTHON=`which python`

pushd Release/OpenVisus 
ConfigureAndTestConda 
DistribToConda 
popd

if [[ "$VISUS_GUI" == "1" ]]; then
	pushd Release.nogui/OpenVisus
	ConfigureAndTestConda
	DistribToConda
	popd
fi

echo "All done"


