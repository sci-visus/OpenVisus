#!/bin/bash

set -ex

BUILD_DIR=${BUILD_DIR:-build_docker}

VISUS_GUI=${VISUS_GUI:-1}

VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}
PYTHON_VERSION=${PYTHON_VERSION:-3.8}
NUMPY_VERSION=${NUMPY_VERSION:-}
Qt5_DIR=${Qt5_DIR:-/opt/qt512}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_TOKEN=${PYPI_TOKEN:-}
ANACONDA_TOKEN=${ANACONDA_TOKEN:-}

DOCKER_TOKEN=${DOCKER_TOKEN:-}
DOCKER_USERNAME=${DOCKER_USERNAME:-}
DOCKER_IMAGE=${DOCKER_IMAGE:-}

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# needs to run using docker
if [[ "$DOCKER_IMAGE" != "" ]] ; then

  # run docker to compile portable OpenVisus
  docker run --rm -v ${PWD}:/home/OpenVisus -w /home/OpenVisus \
    -e BUILD_DIR=build_docker \
    -e PYTHON_VERSION=${PYTHON_VERSION} \
	 -e NUMPY_VERSION=${NUMPY_VERSION} \
    -e PYPI_USERNAME=${PYPI_USERNAME} -e PYPI_TOKEN=${PYPI_TOKEN} \
    -e ANACONDA_TOKEN=${ANACONDA_TOKEN} \
    -e VISUS_GUI=${VISUS_GUI} -e VISUS_SLAM=${VISUS_SLAM} -e VISUS_MODVISUS=${VISUS_MODVISUS} \
    -e INSIDE_DOCKER=1 \
    ${DOCKER_IMAGE} bash scripts/ubuntu.sh

  # now that the wheels/conda are pushed to PyPi and conda, I can build the docker images
  if [[ "${GIT_TAG}" != "" ]] ; then

    # give time to 'receive' the wheel and the conda package
    sleep 90
    
    ARCH=$(uname -m)

    function BuildAndPushDockerImage() {
      pushd $1
      IMAGE=$2
      docker build --tag $IMAGE:${GIT_TAG} --tag $IMAGE:latest --build-arg TAG=${GIT_TAG} --progress=plain ./  
      echo ${DOCKER_TOKEN} | docker login -u=${DOCKER_USERNAME} --password-stdin
      docker push $IMAGE:${GIT_TAG}
      docker push $IMAGE:latest
      popd
    }    

    # modvisus (right now based on python 3.9, change as needed)
    if [[ "${PYTHON_VERSION}" == "3.9" ]] ; then
      BuildAndPushDockerImage Docker/mod_visus visus/mod_visus_$ARCH
    fi

    # jupyter
    # if [[ "${PYTHON_VERSION}" == "3.9" ]] ; then
    #	BuildAndPushDockerImage Docker/jupyter visus/scipy-notebook_$ARCH
    #fi

  fi

  echo "All done ubuntu $PYTHON_VERSION} "
  exit 0

fi


# //////////////////////////////////////////////////////////////
function InstallConda() {
  pushd ~
  curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-$ARCHITECTURE.sh
  bash Miniforge3-Linux-$ARCHITECTURE.sh -b || true # maybe it's already installed?
  rm -f Miniforge3-Linux-$ARCHITECTURE.sh
  popd
}

# //////////////////////////////////////////////////////////////
function ActivateConda() {
   source ~/miniforge3/etc/profile.d/conda.sh || true # can be already activated
   conda config --set always_yes yes --set anaconda_upload no
   conda create --name my-env -c conda-forge python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel 
   conda activate my-env
}

# ///////////////////////////////////////////////
function CreateNonGuiVersion() {
   mkdir -p Release.nogui
   cp -R   Release/OpenVisus Release.nogui/OpenVisus
   rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# ///////////////////////////////////////////////
function ConfigureAndTestCPython() {
   export PYTHONPATH=../
   $PYTHON   -m OpenVisus configure || true # this can fail on linux
   $PYTHON   -m OpenVisus test
   $PYTHON   -m OpenVisus test-gui || true # this can fail on linux
   unset PYTHONPATH
}

# ///////////////////////////////////////////////
function DistribToPip() {
  rm -Rf ./dist
  
  # this fails a LOT on linux
  $PYTHON -m pip install --upgrade pip         ||  true 
  $PYTHON -m pip install setuptools==68.2.0 wheel==0.37.0 cryptography==3.4.0 twine==4.0.2 readme-renderer==41.0 ||  true

  PYTHON_TAG=cp$(echo $PYTHON_VERSION | awk -F'.' '{print $1 $2}')
  $PYTHON setup.py -q bdist_wheel --python-tag=$PYTHON_TAG --plat-name=$PIP_PLATFORM
  
  if [[ "${GIT_TAG}" != "" ]] ; then
    $PYTHON -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing   "dist/*.whl" 
  fi
}

# //////////////////////////////////////////////////////////////
function ConfigureAndTestConda() {
   conda develop $PWD/..
   $PYTHON -m OpenVisus configure || true # this can fail on linux
   $PYTHON -m OpenVisus test
   $PYTHON -m OpenVisus test-gui    || true # this can fail on linux
   conda develop $PWD/.. uninstall
}

# //////////////////////////////////////////////////////////////
function DistribToConda() {
   rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2") || true
   $PYTHON setup.py -q bdist_conda 1>/dev/null
   CONDA_FILENAME=$(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2" | head -n 1)
   if [[ "${GIT_TAG}" != "" ]] ; then
      anaconda --verbose --show-traceback -t ${ANACONDA_TOKEN} upload ${CONDA_FILENAME} --no-progress 
   fi
}


# /////////////////////////////////////////////////////////////////////////
# *** cpython ***
# /////////////////////////////////////////////////////////////////////////

PYTHON=`which python${PYTHON_VERSION}`

# this is for linux/docker (is this needed?)
yum install -y libffi-devel

# make sure pip is updated
${PYTHON} -m pip install --upgrade pip || true

# detect architecture
if [[ "1" == "1" ]]; then
  ARCHITECTURE=`uname -m`
  PIP_PLATFORM=unknown
  if [[ "${ARCHITECTURE}" ==  "x86_64" ]] ; then PIP_PLATFORM=manylinux2010_${ARCHITECTURE} ; fi
  if [[ "${ARCHITECTURE}" == "aarch64" ]] ; then PIP_PLATFORM=manylinux2014_${ARCHITECTURE} ; fi
fi

# compile OpenVisus
if [[ "1" == "1" ]]; then
  mkdir -p ${BUILD_DIR} 
  cd ${BUILD_DIR}
  cmake -DPython_EXECUTABLE=${PYTHON} -DQt5_DIR=${Qt5_DIR} -DVISUS_GUI=${VISUS_GUI} -DVISUS_MODVISUS=${VISUS_MODVISUS} -DVISUS_SLAM=${VISUS_SLAM} ../
  make -j
  make install
fi

if [[ "1" == "1" ]]; then
  pushd Release/OpenVisus
  ConfigureAndTestCPython 
  DistribToPip
  popd
fi

if [[ "${VISUS_GUI}" == "1" ]]; then
  CreateNonGuiVersion
  pushd Release.nogui/OpenVisus 
  ConfigureAndTestCPython 
  DistribToPip 
  popd
fi

# /////////////////////////////////////////////////////////////////////////
# *** conda ***
# /////////////////////////////////////////////////////////////////////////

# scrgiorgio: disabled on linux for now, getting `ImportError: /lib64/libc.so.6: version `GLIBC_2.14' not found1

# install conda
# if [[ "1" == "1" ]]; then
#  # avoid conflicts with pip packages installed using --user
#  export PYTHONNOUSERSITE=True 
#  InstallConda 
#  ActivateConda
#  PYTHON=`which python`
#fi

# fix `bdist_conda` problem
#if [[ "1" == "1" ]]; then
#  pushd ${CONDA_PREFIX}/lib/python${PYTHON_VERSION}
#  cp -n distutils/command/bdist_conda.py site-packages/setuptools/_distutils/command/bdist_conda.py || true
#  popd
#fi

#if [[ "1" == "1" ]]; then
#  pushd Release/OpenVisus 
#  ConfigureAndTestConda 
#  DistribToConda 
#  popd
#fi

#if [[ "${VISUS_GUI}" == "1" ]]; then
#  pushd Release.nogui/OpenVisus
#  ConfigureAndTestConda
#  DistribToConda
#  popd
#fi

#fi

echo "All done ubuntu $PYTHON_VERSION} (in docker) "





