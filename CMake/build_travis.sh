set -ex 

source CMake/build_utils.sh

export VISUS_INTERNAL_DEFAULT=1
export DEPLOY_PYPI=0
export DEPLOY_CONDA=0
export DEPLOY_GITHUB=0

if [[ "$TRAVIS_OS_NAME" == "osx"   ]]; then 
	export COMPILER=clang++ 
fi

if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then 
	export COMPILER=g++-4.9 
fi

if [[ "$CONDA" == "1" ]]; then
	if [[ "${TRAVIS_TAG}" != "" || "${TRAVIS_COMMIT_MESSAGE}" == *"[deploy_conda]"* ]]; then
		echo "Doing deploy to anaconda..."
		export DEPLOY_CONDA=1
	fi
else
	DEPLOY_GITHUB=1
	if [[ "${TRAVIS_TAG}" != "" || "${TRAVIS_COMMIT_MESSAGE}" == *"[deploy_pypi]"* ]]; then
		if [[ $(uname) == "Darwin" || "${DOCKER_IMAGE}" == "quay.io/pypa/manylinux1_x86_64" ]]; then
			echo "Doing deploy to pypi..."
			export DEPLOY_PYPI=1
		fi     
	fi
fi     

if [[ "$CONDA" == "1" ]]; then
	cd $HOME
	InstallMiniconda
	cd "${TRAVIS_BUILD_DIR}"
	cd conda
	conda-build -q openvisus
	conda install -q --use-local openvisus
	export CONDA_BUILD_FILENAME=$(find ${HOME}/miniconda${PYTHON_VERSION:0:1}/conda-bld -iname "openvisus*.tar.bz2")

else
	cd "${TRAVIS_BUILD_DIR}" 
	./build.sh
	cd $HOME
	ActivatePyEnvPython
	export WHEEL_FILENAME=$(find ${TRAVIS_BUILD_DIR}/build/install/dist -iname "*.whl")
	python -m pip install "${WHEEL_FILENAME}" 

fi

# test of it works
python -m OpenVisus configure 
cd $(python -m OpenVisus dirname) 
./visus.sh
python Samples/python/Array.py 
python Samples/python/Dataflow.py 
python Samples/python/Idx.py     

if [[ "${DEPLOY_PYPI}" == "1" ]] ; then 
	DeployPyPi "${WHEEL_FILENAME}" 
fi     

if [[ "${DEPLOY_CONDA}" == "1" ]] ; then 
	anaconda -q -t $ANACONDA_TOKEN upload "${CONDA_BUILD_FILENAME}"
fi

if [[ "${DEPLOY_GITHUB}" != "1" ]] ; then 
	travis_terminate 0
fi  

# Stop error on each command
set +ex  
