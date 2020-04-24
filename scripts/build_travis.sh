#/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

# to test it
# PYTHON_VERSION=3.7 scripts/build_travis.sh

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build_travis}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
BUILD_CONDA=${BUILD_CONDA:-1}

if [ "$(uname)" == "Darwin" ]; then 
	OSX=1
fi

# ////////////////////////////////////////////////////////////////////////
# prerequisites for osx
if (( OSX == 1 )) ; then

	if [ !  -x "$(command -v brew)" ]; then
		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	fi
	
	brew update 1>/dev/null 2>/dev/null || true
	brew install cmake swig  || true
	
	brew install sashkab/python/python@${PYTHON_VERSION} 
	PYTHON_EXECUTABLE=$(brew --prefix python@${PYTHON_VERSION})/bin/python${PYTHON_VERSION}
	
	# some package I need
	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip  || true
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine	 || true
	
	# install Qt (a version which is compatible with PyQt5)
	brew install "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"  
	Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5 
	
	PLATFORM_TAG=macosx_10_9_x86_64
	
	if (( BUILD_CONDA == 1 )) ; then
		./scripts/install/miniconda_osx.sh
		./scripts/install/python.conda.sh ${PYTHON_VERSION}
	fi

else

	PLATFORM_TAG=manylinux2010_x86_64
	
fi

# build OpenVisus
source ./scripts/build.sh

cd ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus

# ////////////////////////////////////////////////////////////////////////
# create the pip wheel

${PYTHON_EXECUTABLE} -m pip install setuptools wheel --upgrade 

rm -Rf ./dist/*OpenVisus-*.whl 
${PYTHON_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=${PLATFORM_TAG}

if [[ "${TRAVIS_TAG}" != ""  && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != "" ]] ; then
	${PYTHON_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi

# ////////////////////////////////////////////////////////////////////////
# build conda
if (( BUILD_CONDA == 1 )) ; then

	# activate 
	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072
	
	conda install -y python=${PYTHON_VERSION}
	conda install anaconda-client setuptools conda-build -y
	
	# remove my Qt (I need to be portable), note sometimes I have seg fault here
	PYTHONPATH=$(pwd)/.. python -m OpenVisus use-pyqt || true 
	
	# create the conda disttribution
	rm -Rf ./dist/* $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	python setup.py -q bdist_conda 
	CONDA_BUILD_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	
	# test it
	if (( 1 == 1 )) ; then
		conda install -y ${CONDA_BUILD_FILENAME}
		pushd $(python -m OpenVisus dirname)
		python Samples/python/Array.py
		python Samples/python/Dataflow.py
		python Samples/python/Idx.py
		popd
	fi
	
	# upload
	if [[ "${TRAVIS_TAG}" != ""  && "${ANACONDA_TOKEN}" != "" ]] ; then
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
	fi

fi






