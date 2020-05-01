#/bin/bash

set -e  # stop or error
set -x  # very verbose

gem install xcpretty 

brew update 1>/dev/null 2>/dev/null || true

# install cmake and swig
brew install cmake swig

# install python
if (( 1 == 1 )) ; then
	brew install sashkab/python/python@${PYTHON_VERSION} 
	export Python_EXECUTABLE==$(brew --prefix python@${PYTHON_VERSION})/bin/python${PYTHON_VERSION}
	${Python_EXECUTABLE} -m pip install -q --upgrade pip                		|| true
	${Python_EXECUTABLE} -m pip install -q numpy setuptools wheel twine --upgrade   || true
fi

# install qt 5.9.3 in order to be compatible with conda PyQt5 (very old so I need to do some tricks)
# NOTE important to keep both the link and real path
if (( 1 == 1 )) ; then
	export Qt5_DIR=/usr/local/opt/qt

	pushd $( brew --prefix )/Homebrew/Library/Taps/homebrew/homebrew-core 
	git fetch --unshallow || true
	git checkout 13d52537d1e0e5f913de46390123436d220035f6 -- Formula/qt.rb 
	cat Formula/qt.rb \
		| sed -e 's|depends_on :mysql|depends_on "mysql-client"|g' \
		| sed -e 's|depends_on :postgresql|depends_on "postgresql"|g' \
		| sed -e 's|depends_on :macos => :mountain_lion|depends_on :macos => :sierra|g' \
		> /tmp/qt.rbscripts/build.sh
	cp /tmp/qt.rb Formula/qt.rb 
	brew install qt 
	brew link --force qt	
	git checkout -f
	popd	
fi
	
# install osx 10.9 for portable binaries
if (( 1 == 1 )) ; then
	export CMAKE_OSX_SYSROOT=~/MacOSX-SDKs/MacOSX10.9.sdk
	pushd ${HOME}
	git clone https://github.com/phracker/MacOSX-SDKs.git 
	popd
fi

# build OpenVisus
mkdir -p build_travis
cd build_travis
	
cmake -GXcode -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} ../
cmake --build ./ --target ALL_BUILD --config Release | xcpretty -c
cmake --build ./ --target install --config Release

# tests
cd build_travis/Release/OpenVisus
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
./visus.command

# wheel
if [[ "${TRAVIS_TAG}" != "" ]] ; then
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=macosx_10_9_x86_64
	WHEEL_FILENAME=dist/OpenVisus-*.whl
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing ${WHEEL_FILENAME}
fi

# conda 
if [[ "${PYTHON_VERSION}" == "3.6" || "${PYTHON_VERSION}" == "3.7" ]] ; then

	pushd ${HOME}
	curl -L --insecure --retry 3 "https://repo.continuum.io/miniconda/Miniconda3-latest-MacOSX-x86_64.sh" -O		
	bash ./Miniconda3-latest-MacOSX-x86_64.sh -b # b stands for silent mode
	popd

	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072

	conda config  --set changeps1 no --set anaconda_upload no
	conda update  --yes conda anaconda-client python=${PYTHON_VERSION} numpy 

	# build openvisus conda
	cd build_travis/Release/OpenVisus
	"${TRAVIS_BUILD_DIR}"/scripts/build_conda.sh

	# upload only if there is a tag
	if [[ "${TRAVIS_TAG}" != "" ]] ; then
		conda install anaconda-client -y
		CONDA_FILENAME=$(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
		anaconda -t ${ANACONDA_TOKEN} upload "${CONDA_FILENAME}"
	fi
fi




