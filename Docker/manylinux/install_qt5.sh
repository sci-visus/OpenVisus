#!/bin/bash

set -e
set -x

source ~/.bashrc 

VERSION=$1
conda activate python-$VERSION
conda install -c conda-forge -y qt==5.12.9 
conda deactivate