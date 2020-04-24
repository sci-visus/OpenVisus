#!/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

if [ "$(uname)" == "Darwin" ]; then 
	OSX=1 
fi

SOURCE_DIR=$(pwd)

# options
BUILD_DIR=${BUILD_DIR:-${SOURCE_DIR}/build}
CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Release}

VISUS_MODVISUS=${VISUS_MODVISUS:-1}
VISUS_GUI=${VISUS_GUI:-1}
PYTHON_VERSION=${PYTHON_VERSION:-3.8.2}

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

declare -a cmake_opts
function AddCMakeOption {
	key=$1
	value=$2
	if [[ "${value}" != "" ]]; then cmake_opts+=(${key}=${value}) ; fi
}

ALL_BUILD_STEP=all
CMAKE_TEST_STEP=test
if (( OSX == 1 )) ; then
	VISUS_MODVISUS=0 # overrule, not supported
	ALL_BUILD_STEP=ALL_BUILD
	CMAKE_TEST_STEP=RUN_TESTS
fi

# try to guess some python stuff
if [[ "${PYTHON_EXECUTABLE}" != ""  ]] ; then

	if [[ "${PYTHON_VERSION}" == "" ]] ; then
		PYTHON_VERSION=$(${PYTHON_EXECUTABLE} -c "import sys; print(sys.version[0:3])")
	fi

	if [[ "${PYTHON_INCLUDE_DIR}" == "" ]] ; then
		PYTHON_INCLUDE_DIR=$(${PYTHON_EXECUTABLE} -c "import sysconfig;print(sysconfig.get_paths()['include'])" )
	fi

	if [[ "${PYTHON_LIBRARY}" == "" ]] ; then

		if (( OSX == 1 )) ; then
			echo ""
			# do I need this?
			#PYTHON_LIBDIR=$(${PYTHON_EXECUTABLE} -c "import sysconfig;print(sysconfig.get_config_var('LIBDIR'))" )
			#PYTHON_LIBRARY=${PYTHON_LIBDIR}/libpython${PYTHON_VERSION}m.dylib
			#if [[ ! -f ${PYTHON_LIBRARY} ]]
			#	PYTHON_LIBRARY=${PYTHON_LIBDIR}/libpython${PYTHON_VERSION}.dylib
			#fi
		else
			PYTHON_LIBRARY=$(${PYTHON_EXECUTABLE} -c "import sysconfig,os;print(os.path.join(sysconfig.get_config_var('LIBDIR'), sysconfig.get_config_var('LDLIBRARY')))")
		fi
	fi

fi

AddCMakeOption -DCMAKE_BUILD_TYPE     "${CMAKE_BUILD_TYPE}"
AddCMakeOption -DSWIG_EXECUTABLE      "${SWIG_EXECUTABLE}"
AddCMakeOption -DPYTHON_VERSION       "${PYTHON_VERSION:0:1}.${PYTHON_VERSION:2:1}"
AddCMakeOption -DPYTHON_EXECUTABLE    "${PYTHON_EXECUTABLE}"
AddCMakeOption -DPYTHON_INCLUDE_DIR   "${PYTHON_INCLUDE_DIR}"
AddCMakeOption -DPYTHON_LIBRARY       "${PYTHON_LIBRARY}"

# mod_visus
AddCMakeOption -DVISUS_MODVISUS "${VISUS_MODVISUS}"
AddCMakeOption -DAPACHE_DIR     "${APACHE_DIR}"

# gui stuff
AddCMakeOption -DVISUS_GUI "${VISUS_GUI}"
AddCMakeOption -DQt5_DIR   "${Qt5_DIR}"

if (( OSX == 1 )) ; then
	cmake_opts+=(-GXcode)
fi

# configure / generate
cmake ${cmake_opts[@]} ${SOURCE_DIR}

InstallDir=$(pwd)/${CMAKE_BUILD_TYPE}/OpenVisus

# ______________________________________________
# build all
if [[ "${TRAVIS_OS_NAME}" == "osx" ]] ; then
	cmake --build ./ --target ${ALL_BUILD_STEP} --config ${CMAKE_BUILD_TYPE} | xcpretty -c
else
	cmake --build ./ --target ${ALL_BUILD_STEP} --config ${CMAKE_BUILD_TYPE}
fi

# ______________________________________________
# install step
if (( 1 == 1 )) ; then
	cmake --build . --target install --config ${CMAKE_BUILD_TYPE}

	# fix permissions (cmake bug)
	pushd ${InstallDir}
	chmod a+rx *.sh *.command 1>/dev/null 2>/dev/null && :
	popd

fi

# ______________________________________________
# cmake test step
if (( 1 == 1 )) ; then
	cmake --build  ./ --target ${CMAKE_TEST_STEP} --config ${CMAKE_BUILD_TYPE}	
fi

# ______________________________________________
# cmake external app
if (( 1 == 1 )) ; then
	cmake --build      ./ --target simple_query    --config ${CMAKE_BUILD_TYPE} 
	if (( VISUS_GUI == 1 )) ; then
		cmake --build   ./ --target simple_viewer2d --config ${CMAKE_BUILD_TYPE} 
	fi	
fi

# ______________________________________________
# do additional tests
if (( 1 == 1  )) ; then

	# test extending python
	pushd ${InstallDir}
	export PYTHONPATH=../
	${PYTHON_EXECUTABLE} -m OpenVisus dirname
	${PYTHON_EXECUTABLE} ${SOURCE_DIR}/Samples/python/Array.py
	${PYTHON_EXECUTABLE} ${SOURCE_DIR}/Samples/python/Dataflow.py
	${PYTHON_EXECUTABLE} ${SOURCE_DIR}/Samples/python/Idx.py
	unset PYTHONPATH
	popd
	
	# test embedding python
	${InstallDir}/visus.sh
fi

# ______________________________________________
if [[ "${TRAVIS_TAG}" != ""  ]] ; then

	${PYTHON_EXECUTABLE} -m pip install setuptools wheel --upgrade 
	
	PLATFORM_TAG=manylinux2010_x86_64
	if (( OSX == 1 )) ; then
		PLATFORM_TAG=macosx_10_9_x86_64
	fi		
	
	pushd ${InstallDir}	
	rm -Rf ./dist/*OpenVisus-*.whl 
	${PYTHON_EXECUTABLE} setup.py -q bdist_wheel --python-tag=cp${PYTHON_VERSION:0:1}${PYTHON_VERSION:2:1} --plat-name=${PLATFORM_TAG}
	${PYTHON_EXECUTABLE} -m twine upload --username ${PYPI_USERNAME} --password ${PYPI_PASSWORD} --skip-existing ${InstallDir}/dist/OpenVisus-*.whl
	popd

fi

# fix permission in case I'm running inside docker
chmod -R a+rwx $(pwd)



