#/bin/bash

# Example:
# PYTHON_VERSION=3.7 scripts/build.sh

 # stop or error
set -e

# very verbose
set -x 

if [[ ${PYTHON_VERSION} == "" ]] ; then
	echo "Please specify PYTHON_VERSION"
	exit 1
fi

if [ "$(uname)" == "Darwin" ]; then 

	CMAKE_OSX_SYSROOT=~/MacOSX-SDKs/MacOSX10.9.sdk
	Qt5_DIR=/usr/local/opt/qt
	PYTHON_EXECUTABLE=/usr/local/opt/python@${PYTHON_VERSION}/bin/python${PYTHON_VERSION}
	
	if [[ ! -x "$(command -v cmake)" ]] ; then
		brew install cmake
	fi
	
	if [[ ! -x "$(command -v swig)" ]] ; then
		brew install swig
	fi	
	
	if [ ! -f "${PYTHON_EXECUTABLE}" ]; then
		brew install sashkab/python/python@${PYTHON_VERSION} 
		${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip  || true
		${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine	 || true
	fi
	
	if [ ! -d "${Qt5_DIR}" ] ; then
		brew install "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"  
	fi
	
	if [ ! -d "${CMAKE_OSX_SYSROOT}" ] ; then
		pushd ~
		git clone https://github.com/phracker/MacOSX-SDKs.git 
		popd
	fi
	
	if [[ ! -d ~/miniconda3 ]] ; then
		pushd ${HOME}
		curl -L --insecure --retry 3 "https://repo.continuum.io/miniconda/Miniconda3-latest-MacOSX-x86_64.sh" -O		
		bash ./Miniconda3-latest-MacOSX-x86_64.sh -b # b stands for silent mode
		rm ./Miniconda3-latest-MacOSX-x86_64.sh
		popd
		
		~/miniconda3/bin/conda config  --set changeps1 no --set anaconda_upload no
		~/miniconda3/bin/conda update  --yes conda
		~/miniconda3/bin/conda install --yes anaconda-client			
		
	fi
	
	# install python inside conda
	./scripts/install/python.conda.sh ${PYTHON_VERSION}
	
	
	
	mkdir -p build_travis
	cd build_travis
	cmake -GXcode -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} -DQt5_DIR=${Qt5_DIR} ../
	cmake --build ./ --target ALL_BUILD --config Release
	cmake --build ./ --target install   --config Release

else

	mkdir -p build_travis
	cd build_travis
	PYTHON_EXECUTABLE=python${PYTHON_VERSION}
	cmake -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DQt5_DIR=/opt/qt59 ../
	cmake --build ./ --target all     --config Release
	cmake --build ./ --target install --config Release

fi

cd Release/OpenVisus

# test
Tests="Array.py Dataflow.py Dataflow2.py Idx.py XIdx.py DataConversion1.py DataConversion2.py" 
for Test in ${Tests} ; do
	PYTHONPATH=../ ${PYTHON_EXECUTABLE} Samples/python/${Test}
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

	${PYTHON_EXECUTABLE} -m pip install setuptools wheel --upgrade || true
	
	if [ "$(uname)" == "Darwin" ]; then 
		PLATFORM_TAG=macosx_10_9_x86_64
	else
		PLATFORM_TAG=manylinux2010_x86_64
	fi	
	
	rm -Rf ./dist/*OpenVisus-*.whl 
	${PYTHON_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=${PLATFORM_TAG}
	WHEEL_FILENAME=dist/OpenVisus-*.whl
	rm -Rf ./build
		
	if [[ "${PYPI_DEPLOY}" != ""  && "${PYPI_USERNAME}" != "" && "${PYPI_PASSWORD}" != "" ]] ; then
		${PYTHON_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing ${WHEEL_FILENAME}
	fi
fi

# ////////////////////////////////////////////////////////////////////////
BUILD_CONDA=${BUILD_CONDA:-1}

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
	CONDA_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	rm -Rf ./build
	
	conda install -y ${CONDA_FILENAME}
	
	for Test in ${Tests} ; do
		python $(python -m OpenVisus dirname)/Samples/python/${Test}
	done
	
	if [[ "${CONDA_DEPLOY}" != ""  && "${ANACONDA_TOKEN}" != "" ]] ; then
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
	fi

fi






