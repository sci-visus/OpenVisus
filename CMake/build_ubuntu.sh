#!/bin/bash

. "$(dirname "$0")/build_common.sh"


# make sure sudo is available
if [ "$EUID" -eq 0 ]; then
	apt-get -qq update
	apt-get -qq install sudo	
fi

sudo apt-get -qq update
sudo apt-get -qq install git 

DetectUbuntuVersion

if (( ${OS_VERSION:0:2}<=14 )); then
	sudo apt-get -qq install software-properties-common
	sudo add-apt-repository -y ppa:deadsnakes/ppa
	sudo apt-get -qq update
fi

sudo apt-get -qq install --allow-unauthenticated cmake swig3.0 git bzip2 ca-certificates build-essential libssl-dev uuid-dev curl automake
sudo apt-get -qq install apache2 apache2-dev

InstallCMake
InstallPatchElf
InstallPython 

if (( VISUS_INTERNAL_DEFAULT == 0 )); then 
	sudo apt-get -qq install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

if (( VISUS_GUI==1 )); then
	if (( ${OS_VERSION:0:2} <=14 )); then
		sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y
		sudo apt-get update -qq
		sudo apt-get install -qq mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev qt59base
		set +e # temporary disable exit
		source /opt/qt59/bin/qt59-env.sh 
		set -e 
	else	
		sudo apt-get install -qq qt5-default qttools5-dev-tools	
	fi
fi

PushCMakeOptions
PushCMakeOption SWIG_EXECUTABLE $(which swig3.0)
cmake ${cmake_opts} ${SOURCE_DIR} 
 
cmake --build . --target all -- -j 4
cmake --build . --target test
cmake --build . --target install 
cmake --build . --target deploy 

pushd install
LD_LIBRARY_PATH=$(pwd):$(dirname ${PYTHON_LIBRARY}) PYTHONPATH=$(pwd) bin/visus     && echo "Embedding working"
LD_LIBRARY_PATH=$(pwd) PYTHONPATH=$(pwd) ${PYTHON_EXECUTABLE} -c "import OpenVisus"	&& echo "Extending working"
popd

if (( DEPLOY_GITHUB == 1 )); then
	cmake --build ./ --target sdist --config ${CMAKE_BUILD_TYPE}
fi




