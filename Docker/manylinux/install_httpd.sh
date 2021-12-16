#!/bin/bash

set -e
set -x

curl -L --insecure --retry 3 "https://github.com/libexpat/libexpat/releases/download/R_2_2_6/expat-2.2.6.tar.bz2" | tar xvjf -
pushd expat* 
./configure  
make 
make install
popd 
rm -Rf expat*

curl -L --insecure --retry 3 "https://ftp.exim.org/pub/pcre/pcre-8.42.tar.gz" | tar xzf -
pushd pcre*  
./configure  
make  
make install 
popd 
rm -Rf pcre*

curl -L --insecure --retry 3 "https://archive.apache.org/dist/httpd/httpd-2.4.38.tar.gz" | tar xzf -
pushd httpd* 
curl -L --insecure --retry 3 "https://archive.apache.org/dist/apr/apr-1.6.5.tar.gz"      | tar xzf - 
mv ./apr-1.6.5      ./srclib/apr

curl -L --insecure --retry 3 "https://archive.apache.org/dist/apr/apr-util-1.6.1.tar.gz" | tar xzf - 
mv ./apr-util-1.6.1 ./srclib/apr-util 

./configure --with-included-apr
make 
make install 
popd

rm -Rf httpd* 

