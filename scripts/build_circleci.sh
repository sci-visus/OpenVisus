#/bin/bash

set -e  # stop or error
set -x  # very verbose

# see my manylinux image
export Python_EXECUTABLE=python${PYTHON_VERSION}  
export Qt5_DIR=/opt/qt59

mkdir build_circleci && cd build_circleci
cmake -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} ../
cmake --build ./ --target all     --config Release
cmake --build ./ --target install --config Release

cd Release/OpenVisus

PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus convert

# wheel
if [[ "${CIRCLE_TAG}" != "" ]] ; then
	${Python_EXECUTABLE} -m pip install setuptools wheel --upgrade || true
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=manylinux2010_x86_64
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi

# conda 
if [[ "${PYTHON_VERSION}" == "3.6" || "${PYTHON_VERSION}" == "3.7" ]] ; then

	if [[ ! -d  ${HOME}/miniconda3 ]]; then 
		pushd ${HOME}
		curl -L --insecure --retry 3 "https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh" -O		
		bash ./Miniconda3-latest-Linux-x86_64.sh -b # b stands for silent mode
		popd
	fi

	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	hash -r
	
	conda config  --set changeps1 no --set anaconda_upload no --set always_yes yes
	conda update   -q conda 
	conda install -q python=${PYTHON_VERSION} numpy

	# I get some random crashes here, so I'm using more actions than a simple configure here..
	conda uninstall -y pyqt || true
	PYTHONPATH=../ python -m OpenVisus remove-qt5
	PYTHONPATH=../ python -m OpenVisus install-pyqt5
	PYTHONPATH=../ python -m OpenVisus link-pyqt5 || true # this crashes on linux (after the sys.exit(0), so I should be fine)
	PYTHONPATH=../ python -m OpenVisus generate-scripts pyqt5

	conda install conda-build -y
	rm -Rf $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")	
	python setup.py -q bdist_conda 

	conda install -y --force-reinstall $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	python -m OpenVisus test
	python -m OpenVisus convert

	# upload only if there is a tag
	if [[ "${CIRCLE_TAG}" != "" && "${ANACONDA_TOKEN}" != "" ]] ; then
		conda install anaconda-client -y
		anaconda -t ${ANACONDA_TOKEN} upload $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	fi
fi


