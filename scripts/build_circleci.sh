#/bin/bash

set -e  # stop or error
set -x  # very verbose

# see my manylinux image
export PYTHON_EXECUTABLE=python${PYTHON_VERSION}  
export Qt5_DIR=/opt/qt59

mkdir -p build_circleci
cd build_circleci
	
cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release
cmake --build ./ --target install --config Release

# tests
cd build_circleci/Release/OpenVisus
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
./visus.sh 


# wheel
if [[ "${CIRCLE_TAG}" !="" ]] ; then
	${Python_EXECUTABLE} -m pip install setuptools wheel --upgrade || true
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=manylinux2010_x86_64
	WHEEL_FILENAME=dist/OpenVisus-*.whl
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing ${WHEEL_FILENAME}
fi

# conda 
if [[ "${PYTHON_VERSION}" == "3.6" || "${PYTHON_VERSION}" == "3.7" ]] ; then

	# install conda
	pushd ${HOME}
	curl -L --insecure --retry 3 "https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh" -O		
	bash ./Miniconda3-latest-Linux-x86_64.sh -b # b stands for silent mode
	popd

	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072
	
	conda config  --set changeps1 no --set anaconda_upload no
	conda update  --yes conda anaconda-client python=${PYTHON_VERSION} numpy

	# build openvisus conda
	cd build_circleci/Release/OpenVisus
	${TRAVIS_BUILD_DIR}/scripts/build_conda.sh

	# upload only if there is a tag
	if [[ "${CIRCLE_TAG}" != "" ]] ; then
		conda install anaconda-client -y
		CONDA_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_FILENAME}"
	fi
fi


