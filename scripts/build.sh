#/bin/bash


# to test it
# PYTHON_VERSION=3.7 scripts/build.sh

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

PYTHON_VERSION=${PYTHON_VERSION:-3.6}

if [ "$(uname)" == "Darwin" ]; then 

	if [ !  -x "$(command -v brew)" ]; then
		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	fi
	
	brew update 1>/dev/null 2>/dev/null || true
	brew install cmake swig || true
	
	# install python
	brew install sashkab/python/python@${PYTHON_VERSION} 
	PYTHON_EXECUTABLE=$(brew --prefix python@${PYTHON_VERSION})/bin/python${PYTHON_VERSION}
	alias python=${PYTHON_EXECUTABLE}
	
	# some package I need
	python -m pip install -q --upgrade pip  || true
	python -m pip install -q numpy setuptools wheel twine	 || true
	
	# qt5 5.11.2
	brew install "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"  
	
	CMAKE_OSX_SYSROOT=~/MacOSX-SDKs/MacOSX10.9.sdk	
	if [ ! -d ${CMAKE_OSX_SYSROOT} ] ; then
		pushd ~
		git clone https://github.com/phracker/MacOSX-SDKs.git 
		popd
	fi
	
	mkdir -p build_travis
	cd build_travis
	cmake -GXcode -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} -DQt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5 ../
	
	if [[ "${TRAVIS_OS_NAME}" == "osx" ]] ; then
		cmake --build ./ --target ALL_BUILD --config Release | xcpretty -c
	else
		cmake --build ./ --target ALL_BUILD --config Release
	fi
	
	cmake --build . --target install   --config Release

else

	PYTHON_EXECUTABLE=python${PYTHON_VERSION}
	alias python=PYTHON_EXECUTABLE
	
	mkdir -p build_travis
	cd build_travis
	cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DQt5_DIR=/opt/qt59 ../
	cmake --build ./ --target all     --config Release
	cmake --build ./ --target install --config Release

fi

cd Release/OpenVisus

# test
Tests="Array.py Dataflow.py Dataflow2.py Idx.py XIdx.py DataConversion1.py DataConversion2.py" 
for Test in ${Tests} ; do
	PYTHONPATH=../ python Samples/python/${Test}
done

if [ "$(uname)" == "Darwin" ]; then
	chmod a+x *.command
	./visus.command
else
	chmod a+x *.sh
	./visus.sh
fi

# ////////////////////////////////////////////////////////////////////////
BUILD_WHEEL=${BUILD_WHEEL:-1}
if (( BUILD_WHEEL == 1 )) ; then

	python -m pip install setuptools wheel --upgrade || true
	
	if [ "$(uname)" == "Darwin" ]; then 
		PLATFORM_TAG=macosx_10_9_x86_64
	else
		PLATFORM_TAG=manylinux2010_x86_64
	fi	
	
	rm -Rf ./dist/*OpenVisus-*.whl 
	python setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=${PLATFORM_TAG}
	WHEEL_FILENAME=dist/OpenVisus-*.whl
	rm -Rf ./build
		
	if [[ "${PYPI_DEPLOY}" != ""  && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != "" ]] ; then
		python -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing ${WHEEL_FILENAME}
	fi
fi

unalias python

# ////////////////////////////////////////////////////////////////////////
BUILD_CONDA=${BUILD_CONDA:-1}
if (( BUILD_CONDA == 1 )) ; then

	if [ "$(uname)" == "Darwin" ]; then 
		./scripts/install/miniconda_osx.sh
		./scripts/install/python.conda.sh ${PYTHON_VERSION}
	fi

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
	CONDA_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	rm -Rf ./build
	
	if (( 1 == 1 )) ; then
		conda install -y ${CONDA_FILENAME}
		
		for Test in ${Tests} ; do
			python $(python -m OpenVisus dirname)/Samples/python/${Test}
		done
	
	fi
	
	if [[ "${CONDA_DEPLOY}" != ""  && "${ANACONDA_TOKEN}" != "" ]] ; then
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
	fi

fi






