#!/bin/bash

INSTALL_DIR=$1

set -e

curl https://www.python.org/ftp/python/3.6.5/Python-3.6.5.tgz -O 
tar xvzf Python-3.6.5.tgz && cd Python-3.6.5  

cat >> Modules/Setup.dist << EOF
SSL=$INSTALL_DIR
_ssl _ssl.c -DUSE_SSL -I$INSTALL_DIR/include -I$INSTALL_DIR/include/openssl -L$INSTALL_DIR/lib -lssl -lcrypto
EOF

./configure --prefix=$INSTALL_DIR --enable-shared 
make -j 4 
make install 

$INSTALL_DIR/bin/pip3 install --upgrade pip 
$INSTALL_DIR/bin/pip3 install numpy PyQt5==5.9 setuptools wheel twine auditwheel 

cd ../ && rm -Rf Python-3.6.5
