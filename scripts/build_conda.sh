#!/bin/bash

set -e  # stop or error
set -x  # very verbose

OSX=0
if [[ "$(uname)" == "Darwin" ]] ; then 
	OSX=1 
fi

conda activate
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no
conda env remove -n tmp || true
conda create -q -n tmp python=${PYTHON_VERSION} numpy
conda activate tmp
conda install conda-build anaconda-client 

if (( OSX == 1 )) ; then
	PYTHONPATH=../ python -m OpenVisus configure
else
	# I get some random crashes in Linux, so I'm using more actions than a simple configure here..
	conda uninstall pyqt || true
	
	PYTHONPATH=../ python -m OpenVisus remove-qt5
	PYTHONPATH=../ python -m OpenVisus install-pyqt5
	PYTHONPATH=../ python -m OpenVisus link-pyqt5 || true # this crashes on linux (after the sys.exit(0), so I should be fine)
	PYTHONPATH=../ python -m OpenVisus generate-scripts pyqt5
	PYTHONPATH=../ python -m OpenVisus test
	PYTHONPATH=../ python -m OpenVisus convert
fi

rm -Rf $(find ~/miniconda3 -iname "openvisus*.tar.bz2")	
python setup.py -q bdist_conda 
CONDA_FILENAME=$(find ~/miniconda3 -iname "openvisus*.tar.bz2"  | head -n 1)

GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)
if [[ "${GIT_TAG}" != "" && "${ANACONDA_TOKEN}" != "" ]] ; then
	anaconda -t "${ANACONDA_TOKEN}" upload "${CONDA_FILENAME}"
fi



