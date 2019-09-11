#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

# cmake flags
export PYTHON_VERSION=${PYTHON_VERSION:-3.6.1}

# assume miniconda installed here
export MINICONDA_ROOT=${HOME}/miniconda${PYTHON_VERSION:0:1}

if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
	export TRAVIS=1 
fi


if [ "$(uname)" == "Darwin" ]; then
	OSX=1
fi

IsRoot=0
if (( EUID== 0 || TRAVIS == 1 )); then 
	IsRoot=1
else
	grep 'docker\|lxc' /proc/1/cgroup && :
	if [ $? == 0 ] ; then  IsRoot=1 ; fi
fi


# install conda
if (( INSTALL_CONDA == 1 )) ; then

	# check I have a portable SDK
	# here I need sudo! 
	if (( OSX ==  1 )) ; then
		if [ ! -d /opt/MacOSX10.9.sdk ] ; then
		  if (( IsRoot == 1 )) ; then
				git clone  https://github.com/phracker/MacOSX-SDKs.git 
				sudo mv MacOSX-SDKs/MacOSX10.9.sdk /opt/
				rm -Rf MacOSX-SDKs
			else
				echo "Missing /opt/MacOSX10.9.sdk, but to install it I need sudo"
				exit -1
			fi
		fi
	fi

	# install Miniconda
	if [ ! -d  ${MINICONDA_ROOT} ]; then
		pushd ${HOME}
		if (( OSX == 1 )) ; then
			__name__=MacOSX-x86_64
		else
			__name__=Linux-x86_64
		fi

		url="https://repo.continuum.io/miniconda/Miniconda${PYTHON_VERSION:0:1}-latest-${__name__}.sh"
		curl -fsSL --insecure ${url} -o miniconda.sh
		chmod +x miniconda.sh
		./miniconda.sh -b
		popd
	fi

	export PATH=${MINICONDA_ROOT}/bin:$PATH
	conda config  --set changeps1 no --set anaconda_upload no
	conda update  --yes conda
	conda install --yes -q conda-build anaconda-client

fi

# activate
export PATH=${MINICONDA_ROOT}/bin:$PATH
conda create  --yes --force --name openvisus-conda python=${PYTHON_VERSION}
source ${MINICONDA_ROOT}/etc/profile.d/conda.sh
# see https://github.com/conda/conda/issues/8072
eval "$(conda shell.bash hook)"
conda activate openvisus-conda
conda install --yes numpy

# see conda/openvisus/build.sh
if (( 1 == 1 )) ; then
	pushd ./conda
	conda-build --python=${PYTHON_VERSION} -q openvisus
	conda install --yes -q --use-local openvisus
	popd
fi

# test
if (( 1 == 1 )) ; then
	pushd $(python -m OpenVisus dirname)
	python Samples/python/Array.py
	python Samples/python/Dataflow.py
	python Samples/python/Idx.py
	popd
fi

# the deploy happens here for the top-level build
if [[ "${TRAVIS_TAG}" != "" ]] ; then
	CONDA_BUILD_FILENAME=$(find ${MINICONDA_ROOT}/conda-bld -iname "openvisus*.tar.bz2")
	echo "Doing deploy to anaconda ${CONDA_BUILD_FILENAME}..."
	anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
fi

echo "OpenVisus CMake/build_conda.sh finished"






