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

# expat
DownloadFile "https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-2.2.6.tar.bz2" 
tar xvjf expat*.bz2 
pushd expat* 
./configure 
make  
make install 
popd
rm -Rf expat*

# pcre
DownloadFile "https://ftp.pcre.org/pub/pcre/pcre-8.42.tar.gz" 
tar xzf pcre*.tar.gz 
pushd pcre* 
./configure 
make 
make install 
popd
rm -Rf pcre*

# httpd
DownloadFile "https://archive.apache.org/dist/httpd/httpd-2.4.38.tar.gz"  
tar xzf httpd*.tar.gz 
pushd httpd* 
DownloadFile "https://archive.apache.org/dist/apr/apr-1.6.5.tar.gz"  
tar xvzf apr-1.6.5.tar.gz 
mv ./apr-1.6.5 ./srclib/apr 
DownloadFile "https://archive.apache.org/dist/apr/apr-util-1.6.1.tar.gz"  
tar xvzf apr-util-1.6.1.tar.gz 
mv ./apr-util-1.6.1 ./srclib/apr-util 
./configure --with-included-apr 
make 
make install 
popd
rm -Rf httpd* 
