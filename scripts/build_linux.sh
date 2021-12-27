#!/bin/bash

__to_test__="
docker run --rm -it \
   -e BUILD_DIR=build_docker \
   -e PYTHON_VERSION=3.8 \
   -e VISUS_GUI=1 -e VISUS_SLAM=1 -e VISUS_MODVISUS=1
   -e PYPI_USERNAME=scrgiorgio -e PYPI_PASSWORD=XXXX \
   -e ANACONDA_TOKEN=ZZZZ \
   -v $PWD:/home/OpenVisus -w /home/OpenVisus \
   visus/portable-linux-binaries_x86_64:4.1 bash

./scripts/build_linux.sh
"

set -e  # stop or error
set -x  # very verbose

BUILD_DIR=${BUILD_DIR:-build_docker}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-1}

ARCHITECTURE=`uname -m`
GIT_TAG=${GIT_TAG:-`git describe --tags --exact-match 2>/dev/null || true`}
if [[ "${ARCHITECTURE}" == "x86_64" ]] ; then
	PIP_PLATFORM=manylinux2010_${ARCHITECTURE}
elif [[ "${ARCHITECTURE}" == "aarch64" ]] ; then
	PIP_PLATFORM=manylinux2014_${ARCHITECTURE}
else
	echo "unknown architecture [${ARCHITECTURE}]"
	exit -1
fi

mkdir -p ${BUILD_DIR} 
cd ${BUILD_DIR} 

CPYTHON=`which python${PYTHON_VERSION}`

# //////////////////////////////////////////////////////////////
function BuildOpenVisus() {
	if [ ! -f ~steps.BuildOpenVisus ] ; then
		cmake -DPython_EXECUTABLE="${CPYTHON}" -DQt5_DIR=/opt/qt512 -DVISUS_GUI=${VISUS_GUI} -DVISUS_MODVISUS=${VISUS_MODVISUS} -DVISUS_SLAM=${VISUS_SLAM} ../
		make -j
		make install
		touch ~steps.BuildOpenVisus
	fi
}

# //////////////////////////////////////////////////////////////
function CreateNonGuiVersion() {
	mkdir -p Release.nogui
	cp -R  Release/OpenVisus Release.nogui/OpenVisus
	rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# //////////////////////////////////////////////////////////////
function ConfigureAndTestCPython() {
	export PYTHONPATH=$PWD/..
	${CPYTHON} -m OpenVisus configure || true  # segmentation fault problem
	${CPYTHON} -m OpenVisus test
	${CPYTHON} -m OpenVisus test-gui  || true # this can fail because OS is not able to run pyqt
	unset PYTHONPATH
}

# //////////////////////////////////////////////////////////////
function DistribToPip() {
	rm -Rf ./dist
	${CPYTHON} -m pip install setuptools wheel twine 1>/dev/null
	${CPYTHON} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=$PIP_PLATFORM	
	if [[ "$GIT_TAG" != "" && "$PYPI_USERNAME" != "" && "$PYPI_PASSWORD" != ""  ]] ; then
		${CPYTHON} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing  "dist/*.whl" 
	fi
}

# //////////////////////////////////////////////////////////////
function InstallConda() {
	if [[ ! -d ~/miniforge3 ]]; then
		pushd ~
		curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-aarch64.sh
		bash Miniforge3-Linux-aarch64.sh -b
		rm -f Miniforge3-Linux-aarch64.sh
		popd
	fi
}

# //////////////////////////////////////////////////////////////
function ActivateConda() {

	source ~/miniforge3/etc/profile.d/conda.sh
	conda config --set always_yes yes --set anaconda_upload no
	conda create --name my-env python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel setuptools
	conda activate my-env

	# fix problem of bdist_conda command not found (I TRIED EVERYTHING! just crazy)
	pushd ${CONDA_PREFIX}/lib/python${PYTHON_VERSION}
	cp -n distutils/command/bdist_conda* site-packages/setuptools/_distutils/command/
	popd

}

# //////////////////////////////////////////////////////////////
function ConfigureAndTestConda() {
	conda develop $PWD/..
	python -m OpenVisus configure || true  # segmentation fault problem
	python -m OpenVisus test
	python -m OpenVisus test-gui  || true  # this can fail because the OS is not able to run pyqt for example
	conda develop $PWD/.. uninstall
}

# //////////////////////////////////////////////////////////////
function DistribToConda() {
	rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2")  || true
	python setup.py -q bdist_conda 1>/dev/null
	if [[ "$GIT_TAG" != "" && "$ANACONDA_TOKEN" != "" ]] ; then
		anaconda --verbose --show-traceback  -t ${ANACONDA_TOKEN}   upload `find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"  | head -n 1`
	fi
}

# //////////////////////////////////////////////////////////////
function Main() {

	# *** cpython ***
	BuildOpenVisus
	
	pushd Release/OpenVisus
	ConfigureAndTestCPython 
	DistribToPip
	popd

	if [[ "$VISUS_GUI" == "1" ]]; then
		CreateNonGuiVersion
		pushd Release.nogui/OpenVisus 
		ConfigureAndTestCPython 
		DistribToPip 
		popd
	fi

	# *** conda ***
	InstallConda
	ActivateConda

	pushd Release/OpenVisus 
	ConfigureAndTestConda 
	DistribToConda 
	popd

	if [[ "$VISUS_GUI" == "1" ]]; then
		pushd Release.nogui/OpenVisus
		ConfigureAndTestConda
		DistribToConda
		popd
	fi

	echo "All done"
}

Main


