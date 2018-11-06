#!/bin/bash
set -ex

mkdir -p /tmp

# install python
cd $HOME     
curl -L https://raw.githubusercontent.com/yyuu/pyenv-installer/master/bin/pyenv-installer -O 
chmod a+x pyenv-installer 
./pyenv-installer 
rm ./pyenv-installer 
export PATH="$HOME/.pyenv/bin:$PATH"
eval "$(pyenv init -)"
eval "$(pyenv virtualenv-init -)"
CONFIGURE_OPTS=--enable-shared pyenv install -s $PYTHON_VERSION    
CONFIGURE_OPTS=--enable-shared pyenv global     $PYTHON_VERSION 
sudo python -m pip install --upgrade pip
sudo python -m pip install --upgrade numpy  

PYTHON_SHORT_VERSION=${PYTHON_VERSION:0:3}
if [[ ${PYTHON_VERSION:0:1} >= 3 ]] ; then PYTHON_M_EXT=m ; else PYTHON_M_EXT= ; fi
PYTHON_EXECUTABLE=$HOME/.pyenv/versions/${PYTHON_VERSION}/bin/python
PYTHON_INCLUDE_DIR=$HOME/.pyenv/versions/${PYTHON_VERSION}/include/python${PYTHON_SHORT_VERSION}${PYTHON_M_EXT}
PYTHON_LIBRARY=$HOME/.pyenv/versions/${PYTHON_VERSION}/lib/libpython${PYTHON_SHORT_VERSION}${PYTHON_M_EXT}.dylib

# this is to solve logs too long 
gem install xcpretty   

#  install dependencies using brew
brew install swig qt5 
QT5_DIR=/usr/local/opt/qt/lib/cmake/Qt5 
if [ $VISUS_INTERNAL_DEFAULT -eq 0 ]; then 
  brew install zlib lz4 tinyxml freeimage openssl curl
fi

# configure OpenVius
cd "${TRAVIS_BUILD_DIR}" 
mkdir build 
cd build 

cmake -GXcode \
  -DPYTHON_VERSION=${PYTHON_VERSION} \
  -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} \
  -DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR} \
  -DPYTHON_LIBRARY=${PYTHON_LIBRARY} \
  -DQt5_DIR=${QT5_DIR} \
  -DPYPI_USERNAME=${PYPI_USERNAME} \
  -DPYPI_PASSWORD=${PYPI_PASSWORD} \
  -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
  ../
	
set -o pipefail &&  cmake --build ./ --target ALL_BUILD   --config $BUILD_TYPE | xcpretty -c

cmake --build ./ --target RUN_TESTS   --config $BUILD_TYPE

# if there is a tag , create the wheel  
if [ -n "$TRAVIS_TAG" ]; then 
  cmake --build ./  --target install         --config $BUILD_TYPE  
  cmake --build ./  --target deploy          --config $BUILD_TYPE 
  cmake --build ./  --target bdist_wheel     --config $BUILD_TYPE 
  cmake --build ./  --target sdist           --config $BUILD_TYPE 
  cmake --build ./  --target pypi            --config $BUILD_TYPE
fi
