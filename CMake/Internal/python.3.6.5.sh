#!/bin/bash

set -e

INSTALL_DIR=$1
OPENSSL_DIR=$2

export LD_LIBRARY_PATH=$OPENSSL_DIR/lib

mkdir -p $INSTALL_DIR
mkdir -p /tmp && cd /tmp
curl https://www.python.org/ftp/python/3.6.5/Python-3.6.5.tgz -O
tar xvzf Python-3.6.5.tgz && cd Python-3.6.5

cat >> Modules/Setup.dist << EOF 
SSL=$OPENSSL_DIR
_ssl _ssl.c -DUSE_SSL -I$OPENSSL_DIR/include -I$OPENSSL_DIR/include/openssl -L$OPENSSL_DIR/lib -lssl -lcrypto
EOF

./configure --prefix=$INSTALL_DIR --enable-shared

echo "!!! Starting python compilation"
make -j 4

echo "!!!! Starting python installation"
make install
$INSTALL_DIR/bin/pip3 install --upgrade pip
$INSTALL_DIR/bin/pip3 install numpy PyQt5==5.9 setuptools wheel twine auditwheel

cd /tmp 
rm -Rf Python-3.6.5
