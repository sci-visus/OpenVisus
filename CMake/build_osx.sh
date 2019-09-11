#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

SOURCE_DIR=$(pwd)
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}

# you can enable/disable certain options 
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}
VISUS_NET=${VISUS_NET:-1}
VISUS_IMAGE=${VISUS_IMAGE:-1}
VISUS_PYTHON=${VISUS_PYTHON:-1}
VISUS_MODVISUS=0 # disabled
VISUS_GUI=${VISUS_GUI:-1}

PYTHON_VERSION=${PYTHON_VERSION:-3.6.6}

# ///////////////////////////////////////////////////////////////////////////////////////////////
function BeginSection {
	set +x	
	echo "//////////////////////////////////////////////////////////////////////// $1"
	set -x
}

# /////////////////////////////////////////////////////////////////////
function AddCMakeOption {
	key=$1
	value=$2
	if [[ "${value}" != "" ]]; then
		cmake_opts+=(${key}=${value})
	fi
}

# /////////////////////////////////////////////////////////////////////
function NeedPython {

	BeginSection InstallPython

	PYTHON_MAJOR_VERSION=${PYTHON_VERSION:0:1}
	PYTHON_MINOR_VERSION=${PYTHON_VERSION:2:1}

	if (( PYTHON_MAJOR_VERSION > 2 )) ; then 
		PYTHON_M_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}m 
	else
		PYTHON_M_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
	fi

	# pyenv does not support 3.7.x  maxosx 10.(12|13)
	#if (( PYTHON_MAJOR_VERSION > 2 )); then
	# scrgiorgio: since I'm having problems with pyenv and python2.7 and libssh I switched to these precompiled homebrew formula
	if (( 1 == 1 )) ; then

		PYTHON_VERSION=${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
		package_name=python${PYTHON_MAJOR_VERSION}${PYTHON_MINOR_VERSION}
		brew install sashkab/python/${package_name} 1>/dev/null && :
		package_dir=$(brew --prefix ${package_name})

		PYTHON_EXECUTABLE=${package_dir}/bin/python${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}
		PYTHON_INCLUDE_DIR=${package_dir}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/include/python${PYTHON_M_VERSION}
		PYTHON_LIBRARY=${package_dir}/Frameworks/Python.framework/Versions/${PYTHON_MAJOR_VERSION}.${PYTHON_MINOR_VERSION}/lib/libpython${PYTHON_M_VERSION}.dylib
		
	else

		brew install readline zlib openssl openssl@1.1 pyenv libffi 1>/dev/null && :

		eval "$(pyenv init -)"
		CONFIGURE_OPTS="--enable-shared" \
		CFLAGS="   -I$(brew --prefix readline)/include -I$(brew --prefix zlib)/include -I$(brew --prefix openssl@1.1)/include" \
		CPPFLAGS=" -I$(brew --prefix readline)/include -I$(brew --prefix zlib)/include -I$(brew --prefix openssl@1.1)/include" \
		LDFLAGS="  -L$(brew --prefix readline)/lib     -L$(brew --prefix zlib)/lib     -L$(brew --prefix openssl@1.1)/lib" \
		pyenv install --skip-existing ${PYTHON_VERSION} 
		pyenv global ${PYTHON_VERSION}
		pyenv rehash
	
		PYTHON_EXECUTABLE=$(pyenv prefix)/bin/python
		PYTHON_INCLUDE_DIR=$(pyenv prefix)/include/python${PYTHON_M_VERSION}
		PYTHON_LIBRARY=$(pyenv prefix)/lib/libpython${PYTHON_M_VERSION}.dylib 	
		
	fi	

	${PYTHON_EXECUTABLE} -m pip install -q --upgrade pip
	${PYTHON_EXECUTABLE} -m pip install -q numpy setuptools wheel twine auditwheel

}

# /////////////////////////////////////////////////////////////////////
function NeedQt5 {

	if [ ! -d /usr/local/Cellar/qt/5.11.2_1 ] ; then
		echo "installing brew Qt5"
		brew uninstall qt5 1>/dev/null && :
		brew install "https://raw.githubusercontent.com/Homebrew/homebrew-core/5eb54ced793999e3dd3bce7c64c34e7ffe65ddfd/Formula/qt.rb"  1>/dev/null && : 
	fi

	Qt5_DIR=$(brew --prefix Qt)/lib/cmake/Qt5
}

if [[ "$TRAVIS_OS_NAME" != "" ]] ; then
	export TRAVIS=1 
fi


#  for travis long log
if (( TRAVIS == 1 )) ; then
	${SudoCmd} gem install xcpretty 
fi 

# brew
if [ !  -x "$(command -v brew)" ]; then
	/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
fi

brew update 1>/dev/null

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

# cmake
brew install cmake 1>/dev/null && :

# python
if (( VISUS_PYTHON == 1 )) ; then
	brew install swig 1>/dev/null && :
	NeedPython
fi

# qt5
if (( VISUS_GUI == 1 )); then
	NeedQt5
fi

declare -a cmake_opts
cmake_opts+=(-GXcode)

AddCMakeOption -DCMAKE_BUILD_TYPE   "${CMAKE_BUILD_TYPE}"
AddCMakeOption -DVISUS_NET          "${VISUS_NET}"
AddCMakeOption -DVISUS_IMAGE        "${VISUS_IMAGE}"

if (( VISUS_PYTHON == 1 )) ; then
	AddCMakeOption -DSWIG_EXECUTABLE "${SWIG_EXECUTABLE}"
fi

# python
AddCMakeOption -DVISUS_PYTHON "${VISUS_PYTHON}"
if (( VISUS_PYTHON == 1 )) ; then
	AddCMakeOption -DPYTHON_VERSION       "${PYTHON_VERSION}"
	AddCMakeOption -DPYTHON_EXECUTABLE    "${PYTHON_EXECUTABLE}"
	AddCMakeOption -DPYTHON_INCLUDE_DIR   "${PYTHON_INCLUDE_DIR}"
	AddCMakeOption -DPYTHON_LIBRARY       "${PYTHON_LIBRARY}"
fi

# qt5
AddCMakeOption -DVISUS_GUI "${VISUS_GUI}"
if (( VISUS_GUI == 1 )); then
	AddCMakeOption -DQt5_DIR "${Qt5_DIR}"
fi

# compile
if (( 1 == 1 )) ; then
	BeginSection "Compiling OpenVisus"
	cmake ${cmake_opts[@]} ${SOURCE_DIR}

	if (( TRAVIS == 1 )) ; then
		cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE} | xcpretty -c
	else
		cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE}
	fi	
fi

# install
if (( 1 == 1 )) ; then
	BeginSection "Installing OpenVisus"	
	cmake --build . --target install --config ${CMAKE_BUILD_TYPE}

	# fix permissions (cmake bug)
	pushd ${CMAKE_BUILD_TYPE}/OpenVisus
	chmod a+rx *.command 1>/dev/null 2>/dev/null && :
	popd

fi

# dist
if (( DEPLOY_PYPI == 1 || DEPLOY_GITHUB == 1 )) ; then
	BeginSection "CMake dist step"
	cmake --build . --target DIST --config ${CMAKE_BUILD_TYPE}
fi

# cmake tests (dont care if it fails)
if (( VISUS_PYTHON == 1 )) ; then
	BeginSection "Test OpenVisus (cmake)"
	cmake --build  ./ --target RUN_TESTS --config ${CMAKE_BUILD_TYPE}	 && :
fi

# cmake external app (dont care if it fails)
if (( 1 == 1 )) ; then
	BeginSection "Test OpenVisus (cmake external app)"
	cmake --build      ./ --target simple_query    --config ${CMAKE_BUILD_TYPE}  && :
	if (( VISUS_GUI == 1 )) ; then
		cmake --build   ./ --target simple_viewer2d --config ${CMAKE_BUILD_TYPE}  && :
	fi	
fi

cd ${HOME}

# test extending python
if (( VISUS_PYTHON == 1  )) ; then
	BeginSection "Test OpenVisus (extending python)"
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}
	${PYTHON_EXECUTABLE} -m OpenVisus dirname
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	${PYTHON_EXECUTABLE} Samples/python/Array.py
	${PYTHON_EXECUTABLE} Samples/python/Dataflow.py
	${PYTHON_EXECUTABLE} Samples/python/Idx.py
	unset PYTHONPATH
	popd
fi

# test embedding python
if (( VISUS_PYTHON == 1 )) ; then
	BeginSection "Test OpenVisus (embedding python)"
	export PYTHONPATH=${BUILD_DIR}/${CMAKE_BUILD_TYPE}
	pushd $(${PYTHON_EXECUTABLE} -m OpenVisus dirname)
	./visus.command
	unset PYTHONPATH
	popd
fi

# deploy pypi
if (( DEPLOY_PYPI == 1 )) ; then
	WHEEL_FILENAME=$(find ${BUILD_DIR}/${CMAKE_BUILD_TYPE}/OpenVisus/dist -iname "*.whl")
	echo "Doing deploy to pypi ${WHEEL_FILENAME}..."
	echo [distutils]                                  > ~/.pypirc
	echo index-servers =  pypi                       >> ~/.pypirc
	echo [pypi]                                      >> ~/.pypirc
	echo username=${PYPI_USERNAME}                   >> ~/.pypirc
	echo password=${PYPI_PASSWORD}                   >> ~/.pypirc
	${PYTHON_EXECUTABLE} -m twine upload --skip-existing "${WHEEL_FILENAME}"
fi

echo "OpenVisus CMake/build_osx.sh finished"




