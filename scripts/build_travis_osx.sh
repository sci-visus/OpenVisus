#/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

cd "${TRAVIS_BUILD_DIR}"

if [ !  -x "$(command -v brew)" ]; then
	/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

brew update 
brew install cmake swig

brew install sashkab/python/python@${PYTHON_VERSION} 
PYTHON_EXECUTABLE=$(brew --prefix python@${PYTHON_VERSION})/bin/python${PYTHON_VERSION}
${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine	

brew install "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"  
Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5 

./scripts/build.sh

# prepare in case of upload to PyPi
cat <<EOF > ~/.pypirc
[distutils]
index-servers =  pypi
[pypi]
username=${PYPI_USERNAME}
password=${PYPI_PASSWORD}
EOF

if [[ "${TRAVIS_TAG}" != "" ]] ; then
	WHEEL_FILENAME=$(find ./build -iname "*.whl")
	${PYTHON_EXECUTABLE} -m twine upload --config-file ~/.pypirc --skip-existing ${WHEEL_FILENAME}
fi





