#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail


# activate python
export PATH=~miniconda3/bin:$PATH
conda install python=${PYTHON_VERSION}
source ~/miniconda3/etc/profile.d/conda.sh
eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072

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







