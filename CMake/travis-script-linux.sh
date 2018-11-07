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
python -m pip install numpy  
PYTHON_SHORT_VERSION=${PYTHON_VERSION:0:3}
if [[ ${PYTHON_VERSION:0:1} > 2 ]] ; then PYTHON_M_EXT=m ; else PYTHON_M_EXT= ; fi
PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_SHORT_VERSION}${PYTHON_M_EXT}
PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_SHORT_VERSION}${PYTHON_M_EXT}.so

#  install patchelf
curl -L https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz -o /tmp/patchelf-0.9.tar.gz
cd /tmp
tar xvzf patchelf-0.9.tar.gz && cd patchelf*
./configure 
make 
sudo make install
export PATH=$PATH:/usr/local/bin

#  install linux dependencies
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get -qy update
sudo apt-get -qy install --allow-unauthenticated swig3.0 git cmake libssl-dev uuid-dev qt5-default qttools5-dev-tools libx11-xcb1 

# install qt 5.9.1
sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y; 
sudo apt-get update -qq
sudo apt-get install -qq qt59base
set +e # temporary disable exit
source /opt/qt59/bin/qt59-env.sh 
set -e 

# if you want to use internal libraries (i.e. less dependencies, slower)
VISUS_INTERNAL_DEFAULT=0

# Download and install recent cmake
CMAKE_URL="http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz"
mkdir -p ${TRAVIS_BUILD_DIR}/deps/cmake
wget --no-check-certificate --quiet -O - ${CMAKE_URL} | tar --strip-components=1 -xz -C ${TRAVIS_BUILD_DIR}/deps/cmake
export PATH=${TRAVIS_BUILD_DIR}/deps/cmake/bin:${PATH} 

# install dependencies
if [ $VISUS_INTERNAL_DEFAULT -eq 0 ]; then 
  sudo apt-get install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

# configure and compile OpenVisus
cd "${TRAVIS_BUILD_DIR}" 
mkdir build 
cd build  
cmake \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DPYTHON_VERSION=${PYTHON_VERSION} \
  -DPYTHON_EXECUTABLE=${PYTHON_EXECUTABLE} \
  -DPYTHON_INCLUDE_DIR=${PYTHON_INCLUDE_DIR} \
  -DPYTHON_LIBRARY=${PYTHON_LIBRARY} \
  -DPYPI_USERNAME=$PYPI_USERNAME \
  -DPYPI_PASSWORD=$PYPI_PASSWORD \
  -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
  ../
  
cmake --build ./ --target all

cmake --build ./ --target test

# if there is a tag, create the wheel and tar.gz
if [ -n "$TRAVIS_TAG" ]; then 
		cmake --build ./ --target install  
		cmake --build ./ --target deploy
		cmake --build ./ --target bdist_wheel
		cmake --build ./ --target sdist 
		# not uploading to PyPi since it will be refused (not using manylinux) 
		# cmake --build ./  --target pypi 
fi

