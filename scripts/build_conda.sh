#!/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

# activate miniconda
export PATH=~/miniconda3/bin:$PATH
source ~/miniconda3/etc/profile.d/conda.sh
eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072

# install python
conda install -y python=${PYTHON_VERSION}

# install build to create the dist
conda install anaconda-client setuptools conda-build -y

# remove my Qt (I need to be portable), note sometimes I have seg fault here
PYTHONPATH=$(pwd)/.. python -m OpenVisus use-pyqt || true 

# create the dist
rm -Rf ./dist/* $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
python setup.py -q bdist_conda 
CONDA_BUILD_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")

# move in the current directory
mv ${CONDA_BUILD_FILENAME} ./
CONDA_BUILD_FILENAME=$(basename "$CONDA_BUILD_FILENAME")

# install dist locally
conda install -y ${CONDA_BUILD_FILENAME}

# test distribution
if (( 1 == 1 )) ; then
	pushd $(python -m OpenVisus dirname)
	python Samples/python/Array.py
	python Samples/python/Dataflow.py
	python Samples/python/Idx.py
	popd
fi

# upload dist
if [[ "${ANACONDA_TOKEN}" != "" || 1 ]] ; then
	anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
fi









