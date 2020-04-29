#/bin/bash

# To test, under osx:
# PYTHON_VERSION=3.7 scripts/build.sh
#
# To test, under linux (assuming you want to use docker):
# sudo docker run -ti -v $(pwd):/root/OpenVisus -w /root/OpenVisus visus/travis-image  /bin/bash
# export PYTHON_VERSION=3.7 
# scripts/build.sh

 # stop or error
set -e

# very verbose
set -x 

if [[ ${PYTHON_VERSION} == "" ]] ; then
	echo "Please specify PYTHON_VERSION"
	exit 1
fi

if [ "$(uname)" == "Darwin" ]; then 

	if [[ ! -x "$(command -v cmake)" ]] ; then
		brew install cmake
	fi
	
	if [[ ! -x "$(command -v swig)" ]] ; then
		brew install swig
	fi	
	
	PYTHON_EXECUTABLE=/usr/local/opt/python@${PYTHON_VERSION}/bin/python${PYTHON_VERSION}
	if [ ! -f "${PYTHON_EXECUTABLE}" ]; then
		brew install sashkab/python/python@${PYTHON_VERSION} 
		${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip  || true
		${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine	 || true
	fi
	
	# 5.9.3 in order to be compatible with conda PyQt5 (very old so I need to do some tricks)
	# NOTE important to keep both the link and real path
	Qt5_DIR=/usr/local/opt/qt
	if [ ! -d "/usr/local/Cellar/qt/5.9.3" ] ; then
		pushd $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core 
		git fetch --unshallow || true
		git checkout 13d52537d1e0e5f913de46390123436d220035f6 -- Formula/qt.rb 
		cat Formula/qt.rb \
			| sed -e 's|depends_on :mysql|depends_on "mysql-client"|g' \
			| sed -e 's|depends_on :postgresql|depends_on "postgresql"|g' \
			| sed -e 's|depends_on :macos => :mountain_lion|depends_on :macos => :sierra|g' \
			> /tmp/qt.rb
		cp /tmp/qt.rb Formula/qt.rb 
		brew install qt 
		brew link --force qt	
		git checkout -f
		popd	
	fi
	
	CMAKE_OSX_SYSROOT=~/MacOSX-SDKs/MacOSX10.9.sdk
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
	
	if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
		cmake -GXcode -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} -DQt5_DIR=${Qt5_DIR} ../ | xcpretty -c
	else
		cmake -GXcode -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} -DQt5_DIR=${Qt5_DIR} ../
	fi
	
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
	
	rm -Rf dist build __pycache__ 
	${PYTHON_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=${PLATFORM_TAG}
	WHEEL_FILENAME=dist/OpenVisus-*.whl
		
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
	
	# create the conda disttribution
	conda install conda-build -y
	rm -Rf dist build __pycache_
	rm -Rf $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")	
	PYTHONPATH=$(pwd)/.. python -m OpenVisus use-pyqt  
	python setup.py -q bdist_conda 
	CONDA_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	
	conda install -y --force-reinstall ${CONDA_FILENAME}
	
	for Test in ${Tests} ; do
		python $(python -m OpenVisus dirname)/Samples/python/${Test}
	done
	
	if [[ "${CONDA_DEPLOY}" != ""  && "${ANACONDA_TOKEN}" != ""  ]] ; then
		conda install anaconda-client -y
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_BUILD_FILENAME}"
	fi

fi






