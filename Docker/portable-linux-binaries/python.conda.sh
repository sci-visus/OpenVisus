#!/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

VERSION=$1

export PATH=${HOME}/miniconda3/bin:$PATH
conda install --yes python=${VERSION}
conda install --yes numpy   

# to activate:
# source ~/miniconda3/etc/profile.d/conda.sh
# eval "$(conda shell.bash hook)" # see https://github.com/conda/conda/issues/8072
