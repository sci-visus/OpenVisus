#!/bin/bash

# ///////////////////////////////////////////////
function CreateNonGuiVersion() {
	mkdir -p Release.nogui
	cp -R	Release/OpenVisus Release.nogui/OpenVisus
	rm -Rf Release.nogui/OpenVisus/QT_VERSION $(find Release.nogui/OpenVisus -iname "*VisusGui*")
}

# ///////////////////////////////////////////////
function ConfigureAndTestCPython() {
	export PYTHONPATH=../
	$PYTHON	-m OpenVisus configure || true # this can fail on linux
	$PYTHON	-m OpenVisus test
	$PYTHON	-m OpenVisus test-gui || true # this can fail on linux
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
	if [[ ! -d "~/miniforge3" ]]; then 
		pushd ~
		curl -L -O https://github.com/conda-forge/miniforge/releases/latest/download/Miniforge3-Linux-$ARCHITECTURE.sh
		bash Miniforge3-Linux-$ARCHITECTURE.sh -b
		rm -f Miniforge3-Linux-$ARCHITECTURE.sh
		popd
	
	fi
}

# //////////////////////////////////////////////////////////////
function ActivateConda() {
	source ~/miniforge3/etc/profile.d/conda.sh || true # can be already activated
	conda config --set always_yes yes --set anaconda_upload no
	conda create --name my-env python=${PYTHON_VERSION} numpy conda anaconda-client conda-build wheel setuptools
	conda activate my-env
}

# //////////////////////////////////////////////////////////////
function ConfigureAndTestConda() {
	conda develop $PWD/..
	$PYTHON -m OpenVisus configure || true # this can fail on linux
	$PYTHON -m OpenVisus test
	$PYTHON -m OpenVisus test-gui	 || true # this can fail on linux
	conda develop $PWD/.. uninstall
}


# ////////////////////////////////////////////////////////////// 
# since bdist_conda is broken, I am switching to conda build
# do just `conda build .`
function CreateCondaMetaFile() {
    if [[ ! -f QT_VERSION ]] ; then 
        PACKAGE_NAME=openvisusnogui
    else
        PACKAGE_NAME=openvisus
    fi

    # The -f option forces a reinstall if a previous version of the package was already installed.
    cat <<EOF > meta.yaml
package:
    name: $PACKAGE_NAME
    version: "$GIT_TAG"
source:
    path: .
build:
    script: python setup.py install -f 
requirements:
    build:
        - python
        - setuptools
    run:
        - python
EOF
}

# ////////////////////////////////////////////////////////////// 
function DistribToConda-CondaBuild() {
	
    # scrgiorgio: don't ask me why I have to do this to avoid errors! 
    rm -Rf ${CONDA_ROOT}/conda-bld $(find ${CONDA_PREFIX} -iname "*openvisus*)  || true 

    CreateCondaMetaFile
    conda build .
    CONDA_FILENAME=`find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"    | head -n 1` 
    if [[ "${GIT_TAG}" != ""    ]] ; then
        anaconda --verbose --show-traceback -t ${ANACONDA_TOKEN} upload ${CONDA_FILENAME}
    fi
}



# //////////////////////////////////////////////////////////////
function DistribToConda() {
	rm -Rf $(find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2")	|| true
	# NOTE: here I am using bdist_conda but sometimes it is broken (e.g. Windows)
	$PYTHON setup.py -q bdist_conda 1>/dev/null
	CONDA_FILENAME=`find ${CONDA_PREFIX} -iname "openvisus*.tar.bz2"	| head -n 1` 
	if [[ "${GIT_TAG}" != ""	]] ; then
		anaconda --verbose --show-traceback	-t ${ANACONDA_TOKEN}	 upload ${CONDA_FILENAME}
	fi
}