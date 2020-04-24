#/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

# to test it
# PYTHON_VERSION=3.7 scripts/build_travis.sh

# avoid collision with other builds
BUILD_DIR=./build_travis

if [ "$(uname)" == "Darwin" ]; then 

	if [ !  -x "$(command -v brew)" ]; then
		/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
	fi
	
	brew update 
	brew install cmake swig
	
	brew install sashkab/python/python@${PYTHON_VERSION} 
	PYTHON_EXECUTABLE=$(brew --prefix python@${PYTHON_VERSION})/bin/python${PYTHON_VERSION}
	
	# some package I need
	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine	
	
	# install Qt (a version which is compatible with PyQt5)
	brew install "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"  
	Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5 
	
	./scripts/build.sh

else

	# see scripts/build_travis.Dockerfile
	
	# cpython
	sudo docker run -ti \
		-v $(pwd):/root/OpenVisus \
		-w /root/OpenVisus \
		-e BUILD_DIR=${BUILD_DIR} \
		-e PYTHON_VERSION=${PYTHON_VERSION} \
		-e PYTHON_EXECUTABLE=/usr/local/bin/python${PYTHON_VERSION} \
		-e TRAVIS_TAG=${TRAVIS_TAG} \
		-e PYPI_USERNAME=${PYPI_USERNAME} \
		-e PYPI_PASSWORD=${PYPI_PASSWORD} \
		-e Qt5_DIR=/opt/qt59 \
		visus/travis-image \
		/bin/bash -c "./scripts/build.sh"
		
	# conda 
	sudo docker run -ti \
		-v $(pwd):/root/OpenVisus \
		-w /root/OpenVisus/${BUILD_DIR}/Release/OpenVisus \
		-e PYTHON_VERSION=${PYTHON_VERSION} \
		-e ANACONDA_TOKEN=${ANACONDA_TOKEN} \
		visus/travis-image \
		/bin/bash -c "/root/OpenVisus/scripts/build_conda.sh"
			
	# reassign permissions in case they changed
	sudo chmod -R a+rwx           ${BUILD_DIR}   
	sudo chown -R "$USER":"$USER" ${BUILD_DIR}

fi






