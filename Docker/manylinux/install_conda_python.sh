#!/bin/bash

set -e
set -x

source ~/.bashrc 

VERSION=$1
conda create --name python-$VERSION -y python=$VERSION
conda activate python-$VERSION
conda install -y numpy anaconda-client 
conda deactivate