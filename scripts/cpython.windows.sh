#!/bin/bash

set -ex

BUILD_DIR=${BUILD_DIR:-build_windows}
PYTHON_VERSION=${PYTHON_VERSION:-3.8}
VISUS_GUI=${VISUS_GUI:-1}
VISUS_SLAM=${VISUS_SLAM:-1}
VISUS_MODVISUS=${VISUS_MODVISUS:-0}
PYPI_USERNAME=${PYPI_USERNAME:-}
PYPI_TOKEN=${PYPI_TOKEN:-}
PIP_PLATFORM=win_amd64

GIT_TAG=`git describe --tags --exact-match 2>/dev/null || true`

# ///////////////////////////////////////////////
# choco install swig
function InstallSwig() {
	mkdir -p /tmp
	pushd /tmp
	__filename=http://prdownloads.sourceforge.net/swig/swigwin-4.0.2.zip
	curl -L --insecure  -O ${__filename} || \
	curl -L --insecure  -O ${__filename} 
	unzip -q swigwin-4.0.2.zip
	SWIG_EXECUTABLE=$PWD/swigwin-4.0.2/swig.exe
	popd
}

# ///////////////////////////////////////////////
# choco install cmake
function InstallCMake() {
	mkdir -p /temp
	pushd /tmp
	__filename=https://github.com/Kitware/CMake/releases/download/v3.22.1/cmake-3.22.1-windows-x86_64.zip
	curl -L ${__filename} -O || \
	curl -L ${__filename} -O
	unzip -q cmake-3.22.1-windows-x86_64.zip 1>/dev/null
	export PATH=$PATH:$PWD/cmake-3.22.1-windows-x86_64/bin
	popd
}

# ///////////////////////////////////////////////
# install from python website for 'all users
function InstallPython() {
	choco install python3 --version=${PYTHON_VERSION} --yes --no-progress --debug --verbose --force # --params "/InstallDir:C:/tmp/python${PYTHON_VERSION}"
	PYTHON=/c/tmp/python${PYTHON_VERSION}/python.exe
}

# ///////////////////////////////////////////////
# choco install qt5-default
function InstallQt5() {	
	$PYTHON -m pip install aqtinstall
	if [[ "$PYTHON_VERSION" == "3.10" ]] ; then
		VERSION=5.15.0
		ARCH=msvc2019_64
	else
		#VERSION=5.12.0
		#ARCH=msvc2017_64
		VERSION=5.15.0
		ARCH=msvc2019_64		
	fi
	$PYTHON -m aqt install-qt --outputdir c:/Qt windows desktop ${VERSION} win64_${ARCH}
	Qt5_Dir=C:/Qt/${VERSION}/${ARCH}/lib/cmake/Qt5
}

# ///////////////////////////////////////////////
function InstallOspray() {
	git clone https://github.com/sci-visus/ospray_win.git ExternalLibs/ospray_win
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
   $PYTHON   -m OpenVisus configure
   $PYTHON   -m OpenVisus test
   $PYTHON   -m OpenVisus test-gui 
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

# install prerequisites
if [[ "1" == "1" ]]; then
	InstallSwig
	InstallCMake
	
	# using existing python
	#InstallPython

	PYTHON=$(which python)
	$PYTHON -m pip install --upgrade pip	
	
	if [[ "$VISUS_GUI" == "1" ]]; then
		# InstallOspray
		InstallQt5
	fi
fi

# compile openvisus
if [[ "1" == "1" ]]; then
	mkdir -p ${BUILD_DIR} 
	cd ${BUILD_DIR}
	cmake \
		-G "Visual Studio 16 2019" \
		-A x64 \
		-DQt5_DIR=${Qt5_Dir} \
		-DPython_EXECUTABLE=${PYTHON} \
		-DSWIG_EXECUTABLE=$SWIG_EXECUTABLE \
		-DVISUS_GUI=$VISUS_GUI \
		-DVISUS_SLAM=$VISUS_SLAM \
		-DVISUS_MODVISUS=$VISUS_MODVISUS \
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

echo "All done windows cpythyon $PYTHON_VERSION} "