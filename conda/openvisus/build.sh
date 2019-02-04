#!/bin/bash

CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-RelWithDebInfo}

mkdir -p build
cd build

if [ $(uname) = "Darwin" ]; then

	cmake -GXcode \
		-DDISABLE_OPENMP=1 \
		-DVISUS_GUI=0 \
		-DVISUS_INTERNAL_DEFAULT=1 \
		-DPYTHON_VERSION=$PY_VER \
		-DPYTHON_EXECUTABLE=$PYTHON \
		../
	
	cmake --build ./ --target ALL_BUILD --config ${CMAKE_BUILD_TYPE}
	cmake --build ./ --target install   --config ${CMAKE_BUILD_TYPE} 		
		
else

	cmake \
		-DDISABLE_OPENMP=1 \
		-DVISUS_GUI=0 \
		-DVISUS_INTERNAL_DEFAULT=1 \
		-DPYTHON_VERSION=$PY_VER \
		-DPYTHON_EXECUTABLE=$PYTHON \	
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \	
		../
		
	cmake --build ./ --target all -- -j 4
	cmake --build ./ --target install
	
fi

pushd install
$PYTHON setup.py install --single-version-externally-managed --record=record.txt
popd