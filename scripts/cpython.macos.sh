#!/bin/bash

set -ex 

BUILD_DIR=${BUILD_DIR:-build_macos}
PYTHON_VERSION=${PYTHON_VERSION:-3.8}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-0}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_TOKEN=${PYPI_TOKEN:-}
PIP_PLATFORM=macosx_10_9_x86_64

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# ///////////////////////////////////////////////
function InstallQt5() {	
	$PYTHON -m pip install aqtinstall
	$PYTHON -m aqt install-qt --outputdir /tmp/Qt mac desktop 5.15.0 clang_64
	find /tmp/Qt/5.15.0
	Qt5_Dir=/tmp/Qt/5.15.0/clang_64/lib/cmake/Qt5
}

# ///////////////////////////////////////////////
function InstallSDK() {
	pushd /tmp 
	rm -Rf MacOSX-SDKs 
	git clone https://github.com/phracker/MacOSX-SDKs.git 1>/dev/null
	export CMAKE_OSX_SYSROOT=$PWD/MacOSX-SDKs/MacOSX10.9.sdk
	popd
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
   $PYTHON -m pip install setuptools wheel twine --upgrade 1>/dev/null || true
   PYTHON_TAG=cp$(echo $PYTHON_VERSION | awk -F'.' '{print $1 $2}')
   $PYTHON setup.py -q bdist_wheel --python-tag=${PYTHON_TAG} --plat-name=$PIP_PLATFORM
   if [[ "${GIT_TAG}" != "" ]] ; then
      $PYTHON -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_TOKEN} --skip-existing   "dist/*.whl" 
   fi
}

# install preconditions
if [[ "1" == "1" ]]; then
	InstallSDK
	brew install swig cmake 
	# brew install python@${PYTHON_VERSION}
	PYTHON=$(which python)
	$PYTHON --version
	$PYTHON -m pip install --upgrade pip
	
	if [[ "$VISUS_GUI" == "1" ]]; then 
		InstallQt5
	fi
fi

# compile openvisus
if [[ "1" == "1" ]]; then

	mkdir -p ${BUILD_DIR} 
	cd ${BUILD_DIR}
	cmake \
		-GXcode \
		-DQt5_DIR=${Qt5_Dir} \
		-DCMAKE_OSX_SYSROOT=$CMAKE_OSX_SYSROOT \
		-DPython_EXECUTABLE=${PYTHON} \
		-DVISUS_GUI=$VISUS_GUI \
		-DVISUS_SLAM=$VISUS_SLAM \
		-DVISUS_MODVISUS=$VISUS_MODVISUS \
		-DVISUS_IDX2=1 \
		../
	cmake --build . --target ALL_BUILD --config Release --parallel 4
	cmake --build . --target install   --config Release
fi

if [[ "1" == "1" ]]; then
	pushd Release/OpenVisus
	ConfigureAndTestCPython
	DistribToPip
	popd
fi

if [[ "$VISUS_GUI" == "1" ]]; then
	CreateNonGuiVersion
	pushd Release.nogui/OpenVisus
	ConfigureAndTestCPython
	DistribToPip
	popd 
fi


echo "All done macos cpythyon $PYTHON_VERSION} "