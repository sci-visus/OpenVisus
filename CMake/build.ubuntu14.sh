#!/bin/bash
set -ex

# if you want to use internal libraries (i.e. less dependencies, slower)
VISUS_INTERNAL_DEFAULT=0

# install python
pushd $HOME
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
popd

#  install patchelf
curl -L https://nixos.org/releases/patchelf/patchelf-0.9/patchelf-0.9.tar.gz -O
tar xvzf patchelf-0.9.tar.gz
pushd patchelf-0.9
./configure --prefix=$(pwd)/install
make 
make install
export PATH=$(pwd)/install/bin:$PATH
popd

#  install linux dependencies (sudo needed)
sudo add-apt-repository -y ppa:deadsnakes/ppa
sudo apt-get -qy update
sudo apt-get -qy install --allow-unauthenticated swig3.0 cmake git

# install qt 5.9.1 (sudo needed)
sudo add-apt-repository ppa:beineri/opt-qt591-trusty -y; 
sudo apt-get update -qq
sudo apt-get install -qq qt59base
set +e # temporary disable exit
source /opt/qt59/bin/qt59-env.sh 
set -e 

# Download and install recent cmake (precompiled)
mkdir -p ./deps/cmake
wget --no-check-certificate --quiet -O - "http://www.cmake.org/files/v3.4/cmake-3.4.3-Linux-x86_64.tar.gz" | tar --strip-components=1 -xz -C ./deps/cmake
export PATH=$(pwd)/deps/cmake/bin:${PATH} 

# install dependencies
if [ $VISUS_INTERNAL_DEFAULT -eq 0 ]; then 
  sudo apt-get install zlib1g-dev liblz4-dev libtinyxml-dev libfreeimage-dev libssl-dev libcurl4-openssl-dev
fi

# configure and compile OpenVisus
if [[ ${PYTHON_VERSION:0:1} > 2 ]] ; then PYTHON_M_EXT=m ; else PYTHON_M_EXT= ; fi
mkdir build 
cd build  
cmake \
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
  -DPYTHON_VERSION=${PYTHON_VERSION} \
  -DPYTHON_EXECUTABLE=$(pyenv prefix)/bin/python \
  -DPYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_VERSION:0:3}${PYTHON_M_EXT} \
  -DPYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_VERSION:0:3}${PYTHON_M_EXT}.so \
  -DPYPI_USERNAME=$PYPI_USERNAME \
  -DPYPI_PASSWORD=$PYPI_PASSWORD \
  -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
  ../
  
cmake --build ./ --target all
cmake --build ./ --target test
cmake --build ./ --target install  
cmake --build ./ --target deploy
cmake --build ./ --target bdist_wheel
cmake --build ./ --target sdist 

if [ -n "$DEPLOY_PYPI" ]; then 
  cmake --build ./  --target pypi 
fi


