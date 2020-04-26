#!/bin/bash


# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

# I need osx 10.9 (need sudo)
if [ ! -d /opt/MacOSX10.9.sdk ] ; then
	git clone https://github.com/phracker/MacOSX-SDKs.git 
	sudo mv MacOSX-SDKs/MacOSX10.9.sdk /opt/
	rm -Rf MacOSX-SDKs
fi

function DownloadFile {
	curl -L --insecure --retry 3 $1 -O		
}


pushd ${HOME}
DownloadFile "https://repo.continuum.io/miniconda/Miniconda3-latest-MacOSX-x86_64.sh"
bash ./Miniconda3-latest-MacOSX-x86_64.sh -b # b stands for silent mode
rm ./Miniconda3-latest-MacOSX-x86_64.sh
popd

export PATH=~/miniconda3/bin:$PATH
conda config  --set changeps1 no --set anaconda_upload no
conda update  --yes conda
conda install --yes anaconda-client




