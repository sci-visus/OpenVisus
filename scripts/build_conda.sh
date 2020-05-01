#/bin/bash

set -e  # stop or error
set -x  # very verbose

# must be executed in OpenVisus binary directory
export PYTHONPATH=$(pwd)/..

# if I don't do this, I get some random crash
conda uninstall -y pyqt || true

# remove Qt5 and use conda pyqt (note sometimes some step crashes, for this reason I split the CONFIGURE in single tasks)
python -m OpenVisus remove-qt5
python -m OpenVisus install-pyqt5
python -m OpenVisus link-pyqt5 || true # this crashes on linux (after the sys.exit(0), so I should be fine)
python -m OpenVisus generate-scripts pyqt5

# create the conda distribution
conda install conda-build -y
rm -Rf $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")	
python setup.py -q bdist_conda 

# install the new file
CONDA_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
conda install -y --force-reinstall ${CONDA_FILENAME}

# test
python -m OpenVisus test
$(python -m OpenVisus dirname)/visus.command


