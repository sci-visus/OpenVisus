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

# this is to solve logs too long 
gem install xcpretty   

#  install dependencies using brew
brew install swig qt5 
if [ $VISUS_INTERNAL_DEFAULT -eq 0 ]; then 
  brew install zlib lz4 tinyxml freeimage openssl curl
fi

# configure OpenVius
if [[ ${PYTHON_VERSION:0:1} > 2 ]] ; then PYTHON_M_EXT=m ; else PYTHON_M_EXT= ; fi
mkdir build 
cd build 
cmake -GXcode \
  -DPYTHON_VERSION=${PYTHON_VERSION} \
  -DPYTHON_EXECUTABLE=$(pyenv prefix)/bin/python \
  -DPYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_VERSION:0:3}${PYTHON_M_EXT} \
  -DPYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_VERSION:0:3}${PYTHON_M_EXT}.dylib \
  -DQt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5 \
  -DPYPI_USERNAME=${PYPI_USERNAME} \
  -DPYPI_PASSWORD=${PYPI_PASSWORD} \
  -DVISUS_INTERNAL_DEFAULT=${VISUS_INTERNAL_DEFAULT} \
  ../
	
set -o pipefail &&  cmake --build ./ --target ALL_BUILD   --config $BUILD_TYPE | xcpretty -c

cmake --build ./ --target RUN_TESTS   --config $BUILD_TYPE
cmake --build ./ --target install     --config $BUILD_TYPE  
cmake --build ./ --target deploy      --config $BUILD_TYPE 
cmake --build ./ --target bdist_wheel --config $BUILD_TYPE 
cmake --build ./ --target sdist       --config $BUILD_TYPE 

if [ -n "$DEPLOY_PYPI" ]; then 
  cmake --build ./ --target pypi  --config $BUILD_TYPE
fi