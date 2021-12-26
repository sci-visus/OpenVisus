#!/bin/bash

# to run:
# PYTHON_VERSION=3.8
# PYPI_PASSWORD=XXXX
# ANACONDA_TOKEN=YYYY
# sudo docker run --rm --platform linux/arm64 -v $PWD:/home/OpenVisus -w /home/OpenVisus -e PYTHON_VERSION=$PYTHON_VERSION -e PYPI_PASSWORD=$PYPI_PASSWORD -e ANACONDA_TOKEN=$ANACONDA_TOKEN  nsdf/manylinux2014_aarch64:latest bash scripts/build_linux.arm64.sh


set -e
set -x

PYTHON_VERSION=${PYTHON_VERSION:-3.8}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_PASSWORD=${PYPI_PASSWORD:-scrgiorgio}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}
BUILD_DIR=${BUILD_DIR:-build_arm64_$PYTHON_VERSION}
PY=/usr/local/bin/python${PYTHON_VERSION}

# since I am manually producing the binaries, I should use the last git tag
GIT_TAG=`$PY ../Libs/swig/setup.py print-tag`

uname -m

mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DPython_EXECUTABLE=/usr/local/bin/python${PYTHON_VERSION} -DVISUS_GUI=0 -DVISUS_SLAM=0 -DVISUS_MODVISUS=0 ../ 
make -j
make install

# configure and test openvisus
if [[ '1' ==  '1']] ; then
  export PYTHONPATH=$PWD/Release
  $PY -m OpenVisus configure || true  # segmentation fault problem on linux
  $PY -m OpenVisus test
  $PY -m OpenVisus test-gui 
  unset PYTHONPATH
fi

# upload wheel
if [[ '1' ==  '1']] ; then
  pushd ./Release/OpenVisus
  $PY -m pip install setuptools wheel twine 1>/dev/null 
  $PY setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=manylinux2014_aarch64
  if [[ '${GIT_TAG}' != '' && '${PYPI_USERNAME}' != '' && '${PYPI_PASSWORD}' != '' ]] ; then
    $PY -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD}  --skip-existing  'dist/*.whl'
  fi
  popd
fi

# install miniconda
if [[ '1' ==  '1']] ; then
  curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-aarch64.sh
  bash Miniforge3-Linux-aarch64.sh -b
  export PATH=~/miniforge3/bin:$PATH
  source /root/miniforge3/etc/profile.d/conda.sh
  conda config --set always_yes yes --set anaconda_upload no
fi

# create conda environment
if [[ '1' ==  '1']] ; then
  conda create -n myenv python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel setuptools
  conda activate myenv

  # fix problem of bdist_conda command not found (I TRIED EVERYTHING! just crazy)
  pushd ${CONDA_PREFIX}/lib/python${PYTHON_VERSION}
  cp -n distutils/command/bdist_conda* site-packages/setuptools/_distutils/command/
  popd
fi

# configure and test openvisus
if [[ '1' ==  '1']] ; then
  conda develop $PWD/Release
  python -m OpenVisus configure
  python -m OpenVisus test
  python -m OpenVisus test-gui 
  conda develop $PWD/Release --uninstall
fi

pushd ./Release/OpenVisus
python setup.py bdist_conda
if [[ "${GIT_TAG}" != ""  && '${ANACONDA_TOKEN}' != ''  ]] ; then
  export PATH=$HOME/anaconda3/bin:$PATH
  anaconda --verbose --show-traceback  -t ${ANACONDA_TOKEN} upload `find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"  | head -n 1`
fi
popd

# finish
if [[ '1' ==  '1']] ; then
  conda deactivate
fi
