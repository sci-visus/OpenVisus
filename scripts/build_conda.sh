#!/bin/bash

# to test:
# cd build/Release/Openvisus diff 
# PYTHON_VERSION=3.7 ./scripts/build_conda.sh

set -e  # stop or error
set -x  # very verbose

export GIT_TAG=${GIT_TAG:-}

OSX=0
if [[ "$(uname)" == "Darwin" ]] ; then OSX=1 ; fi

# see https://docs.conda.io/projects/conda/en/latest/user-guide/tasks/use-conda-with-travis-ci.html
if [[ ! -d  ${HOME}/miniconda3 ]]; then 
	pushd ${HOME}
	__platform__="Linux" 
	if (( OSX == 1 )) ; then __platform__="MacOSX" ; fi
	curl -L --insecure --retry 3 "https://repo.continuum.io/miniconda/Miniconda3-latest-${__platform__}-x86_64.sh" -o	miniconda.sh
	bash ./miniconda.sh -b # b stands for silent mode
	popd
fi

source "$HOME/miniconda3/etc/profile.d/conda.sh"
conda activate

conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
conda update -q conda

conda env remove -n tmp || true
conda create -q -n tmp python=${PYTHON_VERSION} numpy
conda activate tmp
conda install conda-build 

# I get some random crashes here, so I'm using more actions than a simple configure here..
# PYTHONPATH=../ python -m OpenVisus configure
conda uninstall pyqt || true
PYTHONPATH=../ python -m OpenVisus remove-qt5
PYTHONPATH=../ python -m OpenVisus install-pyqt5
PYTHONPATH=../ python -m OpenVisus link-pyqt5 || true # this crashes on linux (after the sys.exit(0), so I should be fine)
PYTHONPATH=../ python -m OpenVisus generate-scripts pyqt5
PYTHONPATH=../ python -m OpenVisus test
PYTHONPATH=../ python -m OpenVisus convert

rm -Rf $(find ~/miniconda3 -iname "openvisus*.tar.bz2")	
python setup.py -q bdist_conda 

# upload only if there is a tag (see https://anaconda.org/ViSUS/settings/access)
if [[ "${GIT_TAG}" != "" && "${ANACONDA_TOKEN}" != "" ]] ; then
	CONDA_FILENAME=$(find ~/miniconda3 -iname "openvisus*.tar.bz2"  | head -n 1)
	echo "CONDA_FILENAME ${CONDA_FILENAME}"
	conda install anaconda-client 
	anaconda -t "${ANACONDA_TOKEN}" upload "${CONDA_FILENAME}"
fi



