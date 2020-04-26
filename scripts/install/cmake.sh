#!/bin/bash

# very verbose
set -x

# stop on error
set -e

# stop of the first command that fails with pipe (|)
set -o pipefail

function DownloadFile {
	curl -L --insecure --retry 3 $1 -O		
}

DownloadFile "https://github.com/Kitware/CMake/releases/download/v3.17.1/cmake-3.17.1-Linux-x86_64.tar.gz"
tar xf cmake*.tar.gz --strip-components=1 -C /usr

rm -R cmake*.tar.gz
