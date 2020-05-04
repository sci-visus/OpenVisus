#/bin/bash

set -e  # stop or error
set -x  # very verbose

gem install xcpretty 
brew update 1>/dev/null 2>/dev/null         || true
brew install cmake  1>/dev/null 2>/dev/null || true
brew install swig   1>/dev/null 2>/dev/null || true

# install python
if (( 1 == 1 )) ; then
	brew install sashkab/python/python@${PYTHON_VERSION} 
	export Python_EXECUTABLE=/usr/local/opt/python@${PYTHON_VERSION}/bin/python${PYTHON_VERSION}
	${Python_EXECUTABLE} -m pip install -q --upgrade pip                		|| true
	${Python_EXECUTABLE} -m pip install -q numpy setuptools wheel twine --upgrade   || true
fi

# install qt 5.9.3 in order to be compatible with conda PyQt5 (very old so I need to do some tricks)
# NOTE important to keep both the link and real path
if (( 1 == 1 )) ; then
	export Qt5_DIR=/usr/local/opt/qt

	pushd $(brew --prefix)/Homebrew/Library/Taps/homebrew/homebrew-core 
	git fetch --unshallow || true
	git checkout 13d52537d1e0e5f913de46390123436d220035f6 -- Formula/qt.rb 
	cat Formula/qt.rb \
		| sed -e 's|depends_on :mysql|depends_on "mysql-client"|g' \
		| sed -e 's|depends_on :postgresql|depends_on "postgresql"|g' \
		| sed -e 's|depends_on :macos => :mountain_lion|depends_on :macos => :sierra|g' \
		> /tmp/qt.rb
	cp /tmp/qt.rb Formula/qt.rb 
	brew install --force qt 
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

mkdir build_travis && cd build_travis
cmake -GXcode -DPython_EXECUTABLE=${Python_EXECUTABLE} -DQt5_DIR=${Qt5_DIR} -DCMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} ../
cmake --build ./ --target ALL_BUILD --config Release | xcpretty -c
cmake --build ./ --target install --config Release

cd Release/OpenVisus

PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus test
PYTHONPATH=../ ${Python_EXECUTABLE} -m OpenVisus convert

# wheel
if [[ "${TRAVIS_TAG}" != "" ]] ; then
	${Python_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=macosx_10_9_x86_64
	${Python_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing dist/OpenVisus-*.whl
fi

# conda 
if [[ "${PYTHON_VERSION}" == "3.6" || "${PYTHON_VERSION}" == "3.7" ]] ; then

	if [[ ! -d  ${HOME}/miniconda3 ]]; then 
		pushd ${HOME}
		curl -L --insecure --retry 3 "https://repo.continuum.io/miniconda/Miniconda3-latest-MacOSX-x86_64.sh" -O		
		bash ./Miniconda3-latest-MacOSX-x86_64.sh -b # b stands for silent mode
		popd
	fi

	export PATH=~/miniconda3/bin:$PATH
	source ~/miniconda3/etc/profile.d/conda.sh
	hash -r

	conda config  --set changeps1 no --set anaconda_upload no --set always_yes yes
	conda update   -q conda 
	conda install  -q python=${PYTHON_VERSION} numpy 

	PYTHONPATH=../ python -m OpenVisus configure
	conda install conda-build -y
	rm -Rf $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")	
	python setup.py -q bdist_conda 

	conda install -y --force-reinstall $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	python -m OpenVisus test
	python -m OpenVisus convert

	if [[ "${TRAVIS_TAG}" != "" && "${ANACONDA_TOKEN}" != "" ]] ; then
		conda install anaconda-client -y
		anaconda -t ${ANACONDA_TOKEN} upload $(find ~/miniconda3/conda-bld -iname "openvisus*.tar.bz2")
	fi
fi




