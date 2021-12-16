#!/bin/bash

set -e
set -x

curl "https://repo.anaconda.com/archive/Anaconda3-2019.07-Linux-$ARCHITECTURE.sh" -o install.sh 
bash install.sh -b  
rm -f install.sh
~/anaconda3/bin/conda init bash 
source ~/.bashrc 
conda update --all -y  
conda config  --set changeps1 no --set anaconda_upload no

