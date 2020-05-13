#!/bin/bash

set -e  # stop or error

export PYTHON_VERSION=${PYTHON_VERSION:-3.6}
export PYTHONUNBUFFERED=1

conda activate                                                                1>/dev/null
conda update -n base -c defaults conda                                        1>/dev/null
conda config --set always_yes yes --set changeps1 no --set anaconda_upload no 1>/dev/null
conda create -q -n tmp python=${PYTHON_VERSION} numpy                         1>/dev/null
conda activate tmp                                                            1>/dev/null
conda install conda-build anaconda-client numpy                               1>/dev/null   

which python

# I get some random crashes in Linux, so I'm using more actions than a simple configure here..
if (( LINUX_FIX_RANDOM_CRASH == 1 )) ; then
	
	conda uninstall pyqt 1>/dev/null 2>/dev/null || true

	PYTHONPATH=../ python -m OpenVisus remove-qt5             1>/dev/null
	PYTHONPATH=../ python -m OpenVisus install-pyqt5          1>/dev/null
	PYTHONPATH=../ python -m OpenVisus link-pyqt5             1>/dev/null 2>/dev/null || true # this crashes on linux (after the sys.exit(0), so I should be fine)
	PYTHONPATH=../ python -m OpenVisus generate-scripts pyqt5 1>/dev/null	
	
else

	PYTHONPATH=../ python -m OpenVisus configure              1>/dev/null
	 
fi

set -x  # very verbose

PYTHONPATH=../ python -m OpenVisus test
PYTHONPATH=../ python -m OpenVisus convert

# https://anaconda.org/ViSUS/settings/access
GIT_TAG=$(git describe --tags --exact-match 2>/dev/null || true)

if [[ "${GIT_TAG}" != "" && "${ANACONDA_TOKEN}" != "" ]] ; then
  rm -Rf $(find ${CONDA} -iname "openvisus*.tar.bz2")  || true
  python setup.py -q bdist_conda 1>/dev/null
  CONDA_FILENAME=$(find ${CONDA} -iname "openvisus*.tar.bz2"  | head -n 1) 
  anaconda --verbose --show-traceback  -t ${ANACONDA_TOKEN} upload "${CONDA_FILENAME}"
fi




           