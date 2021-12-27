#!/bin/bash

# ///////////////////////////////////////////////
function BuildOpenVisusWindows() {
	mkdir -p build 
	cd build
	cmake -G "Visual Studio 16 2019" -A "x64" \
		-DQt5_DIR=${Qt5_DIR} \
		-DPython_EXECUTABLE=${PYTHON}	\
		-DSWIG_EXECUTABLE=${SWIG_EXECUTABLE} \
		../
	cmake --build . --target ALL_BUILD --config Release --parallel 4
	cmake --build . --target install	 --config Release
}

# ///////////////////////////////////////////////
function BuildOpenVisusMacOs() {
	mkdir -p build 
	cd build
	cmake -GXcode	\
		-DQt5_DIR=$Qt5_DIR \
		-DCMAKE_OSX_SYSROOT=$CMAKE_OSX_SYSROOT \
		-DPython_EXECUTABLE=${PYTHON} \
		../
	cmake --build ./ --target ALL_BUILD --config Release --parallel 4 
	cmake --build ./ --target install   --config Release 
} 

# //////////////////////////////////////////////////////////////
function BuildOpenVisusUbuntu() {
	mkdir -p ${BUILD_DIR} 
	cd ${BUILD_DIR} 	
	cmake \
		-DPython_EXECUTABLE="${PYTHON}" \
		-DQt5_DIR=$Qt5_DIR -DVISUS_GUI=${VISUS_GUI} \
		-DVISUS_MODVISUS=${VISUS_MODVISUS} \
		-DVISUS_SLAM=${VISUS_SLAM} \
		../
	make -j
	make install
}


# ///////////////////////////////////////////////
function CreateNonGuiVersion() {
	mkdir -p Release.nogui
	cp -R	Release/OpenVisus Release.nogui/OpenVisus
	rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# ///////////////////////////////////////////////
function ConfigureAndTestCPython() {
	export PYTHONPATH=../
	$PYTHON	-m OpenVisus configure 
	$PYTHON	-m OpenVisus test
	$PYTHON	-m OpenVisus test-gui
	unset PYTHONPATH
}

# ///////////////////////////////////////////////
function DistribToPip() {
	rm -Rf ./dist
	$PYTHON -m pip install setuptools wheel twine --upgrade 1>/dev/null || true
	$PYTHON setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=$PIP_PLATFORM
	if [[ "${GIT_TAG}" != "" ]] ; then
		$PYTHON -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing	"dist/*.whl" 
	fi
}

# //////////////////////////////////////////////////////////////
function InstallCondaUbuntu() {
	pushd ~
	curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-$ARCHITECTURE.sh
	bash Miniforge3-Linux-$ARCHITECTURE.sh -b
	rm -f Miniforge3-Linux-$ARCHITECTURE.sh
	popd
}

# //////////////////////////////////////////////////////////////
function ActivateConda() {
	source ~/miniforge3/etc/profile.d/conda.sh
	conda config --set always_yes yes --set anaconda_upload no
	conda create --name my-env python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel setuptools
	conda activate my-env
}

# //////////////////////////////////////////////////////////////
function ConfigureAndTestConda() {
	conda develop $PWD/..
	$PYTHON -m OpenVisus configure || true # this could fail on linux
	$PYTHON -m OpenVisus test
	$PYTHON -m OpenVisus test-gui	 || true # this could fail on linux
	conda develop $PWD/.. uninstall
}


# //////////////////////////////////////////////////////////////
function DistribToConda() {
	rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2")	|| true
	
	cp -f \
		${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/distutils/command/bdist_conda.py \
		${CONDA_PREFIX}/lib/python${PYTHON_VERSION}/site-packages/setuptools/_distutils/command/bdist_conda.py	
	
	$PYTHON -v setup.py -q bdist_conda 1>/dev/null
	CONDA_FILENAME=`find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"	| head -n 1` 
	if [[ "${GIT_TAG}" != ""	]] ; then
		anaconda --verbose --show-traceback	-t ${ANACONDA_TOKEN}	 upload ${CONDA_FILENAME}
	fi
}		