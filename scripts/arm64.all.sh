#!/bin/bash

set -ex

# rm -Rf ~/miniforge3/envs/arm64-env-*
# rm -Rf ~/cpython/envs/arm64-env*
# rm -Rf ./build*

for it in 3.8 3.9 3.10 3.11 3.12 ;  do

   export PYTHON_VERSION=${it}

   if [ ! -f ~arm64.xcpython.${PYTHON_VERSION}.done ] ; then
      ./scripts/arm64.cpython.sh
      touch ~arm64.cpython.${PYTHON_VERSION}.done
   fi

   if [ ! ~arm64.conda.${PYTHON_VERSION}.done ] ; then
      ./scripts/arm64.conda.sh
      touch ~arm64.conda.${PYTHON_VERSION}.done
   fi

done
